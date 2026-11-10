#include "beatglass/core/file_analysis.hpp"
#include "beatglass/core/causal_replay.hpp"
#include "beatglass/core/audio.hpp"
#include "beatglass/capture/file_reader.hpp"
#include "beatglass/core/playback_session.hpp"
#ifdef BEATGLASS_CLI_MUSIC_REPLAY
#include "beatglass/core/music_replay.hpp"
#endif
#include "state_label.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <shobjidl.h>
#include "../desktop/platform/windows_endpoints.hpp"
#include "../desktop/platform/windows_loopback_session.hpp"
#include "../desktop/platform/windows_playback_session.hpp"
#endif
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <future>
#include <iomanip>
#include <iostream>
#include <memory>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <vector>
#include <mutex>

namespace {
using namespace ftxui;
struct Track { std::filesystem::path path; std::string title; };
std::string format_bpm(const std::optional<float>& bpm) {
  if (!bpm) return "—";
  std::ostringstream out; out << std::fixed << std::setprecision(1) << *bpm; return out.str();
}
Element metric(std::string label, std::string value, Color accent) {
  return vbox({text(std::move(label)) | color(Color::GrayLight) | italic,
               text(std::move(value)) | bold | color(accent)}) | borderRounded | bgcolor(Color::RGB(18, 24, 31)) | flex;
}
struct AnalysisView {
  beatglass::TempoResult result;
  std::vector<float> waveform;
  std::vector<std::uint64_t> beat_frames;
  std::optional<std::string> key;
  std::optional<std::string> chord;
};
struct AnalysisBatch {
  std::vector<std::pair<std::filesystem::path, AnalysisView>> results;
};
std::vector<float> decode_waveform(const std::filesystem::path& path, std::stop_token cancel = {}) {
  beatglass::FileReader reader(path, 1);
  constexpr std::size_t bins = 160;
  std::vector<float> peaks(bins, 0.0f);
  std::vector<float> buffer(4096 * 2);
  std::uint64_t frames_read = 0;
  const auto total_frames = reader.metadata().frames;
  for (;;) {
    if (cancel.stop_requested()) return {};
    const auto read = reader.read(buffer);
    if (read.status == beatglass::FileReadStatus::end) break;
    if (read.status != beatglass::FileReadStatus::data || read.block.format.channels == 0) break;
    const auto channels = read.block.format.channels;
    const auto frames = read.block.samples.size() / channels;
    for (std::size_t frame = 0; frame < frames; ++frame) {
      float peak = 0.0f;
      for (std::size_t channel = 0; channel < channels; ++channel) peak = std::max(peak, std::abs(read.block.samples[frame * channels + channel]));
      const auto bin = total_frames ? std::min<std::size_t>(bins - 1, static_cast<std::size_t>((frames_read + frame) * bins / total_frames)) : 0;
      peaks[bin] = std::max(peaks[bin], peak);
    }
    frames_read += frames;
  }
  return peaks;
}
Element waveform_chart(const std::vector<float>& waveform, const std::vector<std::uint64_t>& beat_frames, std::uint64_t total_frames, float playback_fraction = -1.0f) {
  return vbox({text("WAVEFORM") | bold | underlined | color(Color::GreenLight),
               graph([=](int width, int height) {
                 std::vector<int> values(width);
                 for (int x = 0; x < width; ++x) {
                   const auto sample = waveform.empty() ? 0.08f : waveform[std::min<std::size_t>(waveform.size() - 1, static_cast<std::size_t>(x * waveform.size() / std::max(1, width)))];
                   const auto beat_marker = !waveform.empty() && total_frames > 0 && std::ranges::any_of(beat_frames, [&](const auto frame) { return static_cast<int>(frame * width / total_frames) == x; }) ? 0.12f : 0.0f;
                   const auto cursor = playback_fraction >= 0.0f && x == std::clamp(static_cast<int>(playback_fraction * width), 0, width - 1) ? 0.85f : 0.0f;
                   values[x] = std::clamp(static_cast<int>((sample + beat_marker + cursor) * height), 0, height - 1);
                 }
                 return values;
               }) | color(Color::RGB(99, 255, 183)) | bgcolor(Color::RGB(8, 28, 25)) | borderRounded}) | flex;
}
std::vector<Track> discover(const std::filesystem::path& root, bool recursive = false) {
  std::vector<Track> tracks;
  if (root.empty() || !std::filesystem::is_directory(root)) return tracks;
  const auto visit = [&](const auto& entry) {
    if (!entry.is_regular_file()) return;
    const auto ext = entry.path().extension().string();
    if (ext == ".wav" || ext == ".flac" || ext == ".mp3" || ext == ".aiff" || ext == ".aif") tracks.push_back({entry.path(), entry.path().stem().string()});
  };
  if (recursive) for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) visit(entry);
  else for (const auto& entry : std::filesystem::directory_iterator(root)) visit(entry);
  std::ranges::sort(tracks, {}, &Track::title); return tracks;
}
std::string lower(std::string value) {
  std::ranges::transform(value, value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}
#ifdef _WIN32
std::optional<std::filesystem::path> choose_audio_folder() {
  const auto init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  const auto should_uninitialize = SUCCEEDED(init);
  IFileDialog* dialog = nullptr;
  std::optional<std::filesystem::path> selected;
  if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
    DWORD options = 0;
    if (SUCCEEDED(dialog->GetOptions(&options)) && SUCCEEDED(dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM)) && SUCCEEDED(dialog->Show(nullptr))) {
      IShellItem* item = nullptr;
      if (SUCCEEDED(dialog->GetResult(&item))) {
        PWSTR path = nullptr;
        if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path) {
          selected = std::filesystem::path(path);
          CoTaskMemFree(path);
        }
        item->Release();
      }
    }
    dialog->Release();
  }
  if (should_uninitialize) CoUninitialize();
  return selected;
}
#endif
#ifdef _WIN32
class CliPlayback final {
public:
  CliPlayback() : worker_([this] { run(); }) {}
  ~CliPlayback() {
    { std::lock_guard lock(mutex_); command_ = Command::shutdown; }
    condition_.notify_one();
    worker_.join();
  }
  void play(const std::filesystem::path& path) { { std::lock_guard lock(mutex_); path_ = path; command_ = Command::play; } condition_.notify_one(); }
  void pause() { request(Command::pause); }
  void seek_seconds(std::int64_t seconds) { { std::lock_guard lock(mutex_); seek_delta_seconds_ = seconds; command_ = Command::seek; } condition_.notify_one(); }
  void stop() { request(Command::stop); }
  beatglass::PlaybackState state() const noexcept { return state_.load(std::memory_order_acquire); }
  std::uint64_t epoch() const noexcept { return epoch_.load(std::memory_order_acquire); }
  std::uint64_t current_frame() const noexcept { return current_frame_.load(std::memory_order_acquire); }
  std::uint64_t total_frames() const noexcept { return total_frames_.load(std::memory_order_acquire); }
  std::string status() const { std::lock_guard lock(mutex_); return status_; }
private:
  enum class Command { none, play, pause, seek, stop, shutdown };
  void request(Command command) { { std::lock_guard lock(mutex_); command_ = command; } condition_.notify_one(); }
  Command take_command() { std::lock_guard lock(mutex_); const auto command = command_; command_ = Command::none; return command; }
  std::int64_t take_seek_delta_seconds() { std::lock_guard lock(mutex_); const auto delta = seek_delta_seconds_; seek_delta_seconds_ = 0; return delta; }
  void set_status(std::string status) { std::lock_guard lock(mutex_); status_ = std::move(status); }
  void run() {
    std::unique_ptr<beatglass::FileReader> reader;
    beatglass::windows::WindowsPlaybackSession output;
    std::vector<float> decoded(4096 * 2), render(4096 * 2);
    std::uint64_t next_epoch = 0;
    bool shutdown = false;
    std::uint64_t current_frame = 0;
    while (!shutdown) {
      auto command = take_command();
      if (command == Command::shutdown) break;
      if (command == Command::play) {
        output.stop(); reader.reset(); state_.store(beatglass::PlaybackState::stopped, std::memory_order_release);
        try {
          reader = std::make_unique<beatglass::FileReader>(path_, ++next_epoch);
          current_frame = 0;
          std::string error;
          const auto& metadata = reader->metadata();
          current_frame_.store(0, std::memory_order_release);
          total_frames_.store(metadata.frames, std::memory_order_release);
          if (!output.start(metadata.sample_rate, static_cast<std::uint16_t>(metadata.channels), decoded.size(), next_epoch, error)) { set_status("Playback unavailable: " + error); reader.reset(); continue; }
          epoch_.store(next_epoch, std::memory_order_release); state_.store(beatglass::PlaybackState::playing, std::memory_order_release); set_status("Playing decoded PCM through native output.");
        } catch (const std::exception& error) { set_status(std::string("Playback failed: ") + error.what()); reader.reset(); continue; }
      } else if (command == Command::pause) {
        if (output.pause()) { state_.store(beatglass::PlaybackState::paused, std::memory_order_release); set_status("Playback paused."); }
      } else if (command == Command::seek && reader) {
        try {
          const auto metadata = reader->metadata();
          const auto delta_seconds = take_seek_delta_seconds();
          const auto delta = delta_seconds * static_cast<std::int64_t>(metadata.sample_rate);
          const auto target = delta < 0 && current_frame < static_cast<std::uint64_t>(-delta)
              ? 0 : delta < 0 ? current_frame - static_cast<std::uint64_t>(-delta)
              : std::min<std::uint64_t>(metadata.frames, current_frame + static_cast<std::uint64_t>(delta));
          output.stop(); reader->seek(target, ++next_epoch);
          std::string error;
          if (!output.start(metadata.sample_rate, static_cast<std::uint16_t>(metadata.channels), decoded.size(), next_epoch, error)) throw std::runtime_error(error);
          current_frame = target; epoch_.store(next_epoch, std::memory_order_release); state_.store(beatglass::PlaybackState::playing, std::memory_order_release); set_status("Playback seeked to frame " + std::to_string(target) + ".");
          current_frame_.store(target, std::memory_order_release);
        } catch (const std::exception& error) { set_status(std::string("Playback seek failed: ") + error.what()); }
      } else if (command == Command::stop) {
        output.stop(); reader.reset(); current_frame_.store(0, std::memory_order_release); total_frames_.store(0, std::memory_order_release); state_.store(beatglass::PlaybackState::stopped, std::memory_order_release); set_status("Playback stopped.");
      }
      if (!reader || state_.load(std::memory_order_acquire) != beatglass::PlaybackState::playing) {
        std::unique_lock lock(mutex_); condition_.wait_for(lock, std::chrono::milliseconds(20), [&] { return command_ != Command::none; }); continue;
      }
      const auto read = reader->read(decoded);
      if (read.status == beatglass::FileReadStatus::end) { output.stop(); reader.reset(); state_.store(beatglass::PlaybackState::stopped, std::memory_order_release); set_status("Playback reached end of file."); continue; }
      if (read.status != beatglass::FileReadStatus::data) continue;
      current_frame = read.block.first_frame + read.block.samples.size() / std::max<std::size_t>(1, read.block.format.channels);
      current_frame_.store(current_frame, std::memory_order_release);
      while (!output.submit(read.block)) {
        output.render(render);
        command = take_command();
        if (command == Command::stop || command == Command::shutdown) { output.stop(); reader.reset(); state_.store(beatglass::PlaybackState::stopped, std::memory_order_release); shutdown = command == Command::shutdown; break; }
        if (command == Command::pause) { output.pause(); state_.store(beatglass::PlaybackState::paused, std::memory_order_release); set_status("Playback paused."); break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
      }
      if (shutdown || !reader || state_.load(std::memory_order_acquire) != beatglass::PlaybackState::playing) continue;
      output.render(render);
    }
    output.stop();
  }
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::filesystem::path path_;
  Command command_{Command::none};
  std::string status_{"Playback stopped."};
  std::thread worker_;
  std::atomic<beatglass::PlaybackState> state_{beatglass::PlaybackState::stopped};
  std::atomic<std::uint64_t> epoch_{};
  std::atomic<std::uint64_t> current_frame_{};
  std::atomic<std::uint64_t> total_frames_{};
  std::int64_t seek_delta_seconds_{};
};
#endif
}  // namespace

int main(int argc, char** argv) {
  using namespace ftxui;
  if (argc == 2 && (std::string_view(argv[1]) == "--version" || std::string_view(argv[1]) == "-V")) {
    std::cout << "beatglass-cli " << BEATGLASS_VERSION << "\n";
    return 0;
  }
  if (argc == 2 && (std::string_view(argv[1]) == "--help" || std::string_view(argv[1]) == "-h")) {
    std::cout << "Usage: beatglass-cli [audio-folder]\n"
                 "       beatglass-cli --snapshot <audio-folder> [tab]\n"
                 "\nTabs: 1/library  2/live  3/settings\n";
    return 0;
  }
  const bool snapshot = argc > 1 && std::string(argv[1]) == "--snapshot";
  std::filesystem::path root = snapshot ? (argc > 2 ? argv[2] : std::filesystem::current_path()) : (argc > 1 ? argv[1] : std::filesystem::current_path());
  auto tracks = discover(root, true);
  bool sort_by_path = false;
  std::string folder_text = root.string();
  std::string filter;
  bool recursive = true;
  std::vector<std::string> live_endpoints;
  std::vector<std::string> live_endpoint_ids;
  std::string live_error;
  float live_bpm_multiplier = 1.0f;
#ifdef _WIN32
  auto live_endpoint_snapshot = beatglass::windows::enumerate_render_endpoints();
  live_error = live_endpoint_snapshot.error;
  for (const auto& endpoint : live_endpoint_snapshot.endpoints) {
    live_endpoints.push_back(endpoint.name + (endpoint.is_default ? " (default)" : ""));
    live_endpoint_ids.push_back(endpoint.id);
  }
#else
  live_endpoints.push_back("PipeWire/Core Audio/WASAPI adapter");
  live_error = "Native sink enumeration is platform-specific and not enabled in this build.";
#endif
  int tab = 0, selected = tracks.empty() ? -1 : 0;
  int live_selected = 0;
#ifdef _WIN32
  for (int index = 0; index < static_cast<int>(live_endpoint_snapshot.endpoints.size()); ++index) if (live_endpoint_snapshot.endpoints[index].is_default) live_selected = index;
#endif
  std::string status = tracks.empty() ? "Choose a folder with audio files to begin." : "Ready to analyze a track.";
  std::optional<beatglass::TempoResult> result;
  std::vector<float> waveform;
  std::vector<std::uint64_t> beat_frames;
  std::optional<std::string> key;
  std::optional<std::string> chord;
  std::filesystem::path analysis_path;
  bool busy = false;
  bool reduce_motion = false;
#ifdef _WIN32
  CliPlayback playback;
#else
  beatglass::PlaybackSession playback;
#endif
  std::uint64_t playback_epoch = 0;
  int spinner_frame = 0;
  std::future<AnalysisBatch> analysis_future;
  std::stop_source analysis_stop;
  std::atomic_bool analysis_cancelled{false};
  std::atomic<std::uint64_t> analysis_frames{0};
  std::atomic<std::uint64_t> analysis_total{0};
  std::atomic<std::size_t> batch_completed{0};
  std::atomic<std::size_t> batch_total{0};
  std::map<std::filesystem::path, AnalysisView> completed_analyses;
  std::vector<std::string> library_labels;
  std::vector<int> library_indices;
  int library_visible_selected = 0;
#ifdef _WIN32
  beatglass::SourceEpochAllocator live_epochs;
  std::unique_ptr<beatglass::windows::LoopbackSession> live_session;
  std::unique_ptr<beatglass::CausalTempoReplay> live_replay = std::make_unique<beatglass::CausalTempoReplay>();
  std::string live_status = "Capture stopped.";
  std::optional<float> live_bpm;
  beatglass::TrackingState live_tracking_state = beatglass::TrackingState::stopped;
  std::uint32_t live_sample_rate = 0;
  std::uint64_t live_observed_frames = 0;
  std::uint64_t live_discontinuities = 0;
  std::vector<float> live_waveform(160, 0.0f);
  live_session = std::make_unique<beatglass::windows::LoopbackSession>(live_epochs, [&] {
    live_replay = std::make_unique<beatglass::CausalTempoReplay>();
    live_bpm.reset();
    live_tracking_state = beatglass::TrackingState::reacquiring;
    live_sample_rate = 0; live_observed_frames = 0; live_discontinuities = 0;
  });
#else
  std::string live_status = "Native loopback capture is unavailable in this build.";
  std::optional<float> live_bpm;
  beatglass::TrackingState live_tracking_state = beatglass::TrackingState::stopped;
  std::uint32_t live_sample_rate = 0;
  std::uint64_t live_observed_frames = 0;
  std::uint64_t live_discontinuities = 0;
  std::vector<float> live_waveform(160, 0.0f);
#endif
  auto screen = ScreenInteractive::TerminalOutput();
  auto analyze_paths = [&](std::vector<std::filesystem::path> paths) {
    if (busy) {
      status = "Analysis already in progress; cancel it before starting another track.";
      return;
    }
    if (paths.empty()) {
      status = "Select a supported audio file first.";
      return;
    }
    busy = true; analysis_cancelled.store(false, std::memory_order_release); analysis_stop = std::stop_source{}; analysis_frames.store(0); analysis_total.store(0); batch_completed.store(0); batch_total.store(paths.size()); status = paths.size() == 1 ? "Analyzing " + paths.front().stem().string() + "…" : "Analyzing " + std::to_string(paths.size()) + " tracks…";
    const auto focus_path = selected >= 0 && selected < static_cast<int>(tracks.size()) ? tracks[selected].path : paths.front();
    analysis_path = focus_path;
    const auto stop_token = analysis_stop.get_token();
    analysis_future = std::async(std::launch::async, [paths = std::move(paths), stop_token, &analysis_frames, &analysis_total, &batch_completed] {
      AnalysisBatch batch;
      for (const auto& path : paths) {
        if (stop_token.stop_requested()) break;
        const auto metadata = beatglass::read_metadata(path);
        analysis_total.store(metadata.frames, std::memory_order_release);
        std::vector<std::uint64_t> beat_frames;
        std::optional<std::string> key;
        std::optional<std::string> chord;
#ifdef BEATGLASS_CLI_MUSIC_REPLAY
        const auto tonal = beatglass::replay_music_file(path, 1, [&](const beatglass::CausalMusicBeat& beat) { beat_frames.push_back(beat.frame); }, stop_token,
          [&](const beatglass::CausalTonalFrame& frame) { key = frame.rolling_key; chord = frame.local_chord; },
          [&](const beatglass::CausalMusicState& state) { analysis_frames.store(state.tempo.observed_through, std::memory_order_release); });
        const beatglass::TempoResult result{metadata, tonal.tempo.bpm, static_cast<unsigned>(std::min<std::uint64_t>(tonal.tempo.beats, std::numeric_limits<unsigned>::max())), tonal.tempo.state};
#else
        const auto result = beatglass::replay_file(path, [&](std::uint64_t observed, const beatglass::TempoResult&) { analysis_frames.store(observed, std::memory_order_release); }, [&](const beatglass::ReplayBeat& beat) { beat_frames.push_back(beat.frame); }, stop_token);
#endif
        auto waveform = decode_waveform(path, stop_token);
        if (stop_token.stop_requested()) break;
        batch.results.emplace_back(path, AnalysisView{result, std::move(waveform), std::move(beat_frames), std::move(key), std::move(chord)});
        batch_completed.fetch_add(1, std::memory_order_acq_rel);
      }
      return batch;
    });
  };
  auto analyze = [&] {
    if (selected < 0 || selected >= static_cast<int>(tracks.size())) { status = "Select a supported audio file first."; return; }
    analyze_paths({tracks[selected].path});
  };
  auto refresh = [&] {
    const auto cancelling = busy;
    const auto previous_path = selected >= 0 && selected < static_cast<int>(tracks.size())
        ? tracks[static_cast<std::size_t>(selected)].path : std::filesystem::path{};
    if (cancelling) {
      analysis_cancelled.store(true, std::memory_order_release);
      analysis_stop.request_stop();
    }
    tracks = discover(root, recursive);
    if (sort_by_path) std::ranges::sort(tracks, [](const Track& left, const Track& right) { return left.path.generic_string() < right.path.generic_string(); });
    selected = -1;
    if (!previous_path.empty()) {
      for (int index = 0; index < static_cast<int>(tracks.size()); ++index) {
        if (tracks[static_cast<std::size_t>(index)].path == previous_path) { selected = index; break; }
      }
    }
    if (selected < 0 && !tracks.empty()) selected = 0;
    result.reset(); waveform.clear(); beat_frames.clear(); key.reset(); chord.reset();
    completed_analyses.clear();
    status = std::to_string(tracks.size()) + " tracks indexed; selection preserved by path." +
        (cancelling ? " Cancelling the previous analysis." : "");
  };
  auto filter_input = Input(&filter, "Filter songs");
  auto folder_input = Input(&folder_text, "Audio folder");
  std::string seek_step_text = "5";
  auto seek_step_input = Input(&seek_step_text, "seconds");
  auto seek_step = [&] {
    try { return std::clamp(std::stoi(seek_step_text), 1, 60); }
    catch (...) { return 5; }
  };
  auto recursive_checkbox = Checkbox("Recursive", &recursive);
  auto refresh_button = Button(" Refresh ", refresh);
  std::string sort_button_label = " Sort: title ";
  auto sort_button = Button(&sort_button_label, [&] {
    sort_by_path = !sort_by_path;
    sort_button_label = sort_by_path ? " Sort: path " : " Sort: title ";
    refresh();
    status = std::string("Sorted visible Library by ") + (sort_by_path ? "path." : "title.");
  });
  auto cancel_analysis_button = Button(" Cancel analysis ", [&] {
    if (!busy) return;
    analysis_cancelled.store(true, std::memory_order_release);
    analysis_stop.request_stop();
    status = "Cancelling analysis…";
  });
  auto analyze_all_button = Button(" Analyze all ", [&] {
    std::vector<std::filesystem::path> paths;
    for (const auto index : library_indices) paths.push_back(tracks[static_cast<std::size_t>(index)].path);
    analyze_paths(std::move(paths));
  });
  auto open_folder_button = Button(" Open folder ", [&] {
    const std::filesystem::path requested(folder_text);
    std::error_code error;
    if (!std::filesystem::is_directory(requested, error)) { status = "Folder unavailable: " + folder_text; return; }
    root = requested; refresh(); status = "Opened " + root.string() + " — " + std::to_string(tracks.size()) + " tracks indexed.";
  });
  auto choose_folder_button = Button(" Choose folder… ", [&] {
#ifdef _WIN32
    if (const auto chosen = choose_audio_folder()) {
      folder_text = chosen->string();
      root = *chosen;
      refresh();
      status = "Opened " + root.string() + " — " + std::to_string(tracks.size()) + " tracks indexed.";
    } else {
      status = "Folder selection cancelled; current Library folder is unchanged.";
    }
#else
    status = "Native folder picker is unavailable; edit the path and press Open folder.";
#endif
  });
  auto play_button = Button(" Play ", [&] {
    if (selected < 0 || selected >= static_cast<int>(tracks.size())) {
      status = "Select a supported audio file before starting playback.";
      return;
    }
#ifdef _WIN32
    playback.play(tracks[selected].path);
    status = "Playing " + tracks[selected].title + " through the selected output.";
#else
    status = "Native playback is unavailable in this build; analysis remains available.";
#endif
  });
  auto pause_button = Button(" Pause ", [&] {
#ifdef _WIN32
    playback.pause();
    status = "Playback pause requested.";
#else
    if (playback.pause()) status = "Playback paused.";
#endif
  });
  auto stop_button = Button(" Stop ", [&] {
    playback.stop();
    status = "Playback stopped.";
  });
  auto select_relative = [&](int direction) {
    if (library_indices.empty()) return;
    auto current = std::find(library_indices.begin(), library_indices.end(), selected);
    auto visible = current == library_indices.end() ? 0 : static_cast<int>(std::distance(library_indices.begin(), current));
    visible = (visible + direction + static_cast<int>(library_indices.size())) % static_cast<int>(library_indices.size());
    const auto cancelling = busy;
    if (cancelling) {
      analysis_cancelled.store(true, std::memory_order_release);
      analysis_stop.request_stop();
    }
    selected = library_indices[static_cast<std::size_t>(visible)];
    library_visible_selected = visible;
    result.reset(); waveform.clear(); beat_frames.clear(); key.reset(); chord.reset();
    if (const auto cached = completed_analyses.find(tracks[selected].path); cached != completed_analyses.end()) {
      result = cached->second.result;
      waveform = cached->second.waveform;
      beat_frames = cached->second.beat_frames;
      key = cached->second.key; chord = cached->second.chord;
    }
    status = "Selected " + tracks[selected].title + " — press Enter to analyze or Space to play." +
        (cancelling ? " Cancelling analysis because the selected track changed." : "");
  };
  auto previous_button = Button(" Prev ", [&] { select_relative(-1); });
  auto next_button = Button(" Next ", [&] { select_relative(1); });
  auto seek_back_button = Button(" − ", [&] {
#ifdef _WIN32
    playback.seek_seconds(-seek_step());
    status = "Playback seek backward requested.";
#else
    status = "Native playback seek is unavailable in this build.";
#endif
  });
  auto seek_forward_button = Button(" + ", [&] {
#ifdef _WIN32
    playback.seek_seconds(seek_step());
    status = "Playback seek forward requested.";
#else
    status = "Native playback seek is unavailable in this build.";
#endif
  });
  MenuOption library_menu_option;
  library_menu_option.on_change = [&] {
    if (library_visible_selected < 0 || library_visible_selected >= static_cast<int>(library_indices.size())) return;
    const auto next = library_indices[static_cast<std::size_t>(library_visible_selected)];
    if (next == selected) return;
    const auto cancelling = busy;
    if (cancelling) {
      analysis_cancelled.store(true, std::memory_order_release);
      analysis_stop.request_stop();
    }
    selected = next;
    result.reset(); waveform.clear(); beat_frames.clear(); key.reset(); chord.reset();
    if (const auto cached = completed_analyses.find(tracks[selected].path); cached != completed_analyses.end()) {
      result = cached->second.result;
      waveform = cached->second.waveform;
      beat_frames = cached->second.beat_frames;
      key = cached->second.key; chord = cached->second.chord;
    }
    status = "Selected " + tracks[selected].title + " — press Enter to analyze or Space to play." +
        (cancelling ? " Cancelling analysis because the selected track changed." : "");
  };
  auto library_menu = Menu(&library_labels, &library_visible_selected, library_menu_option);
  auto library_controls = Container::Vertical({Container::Horizontal({folder_input, choose_folder_button, open_folder_button, recursive_checkbox, sort_button, refresh_button}), Container::Horizontal({filter_input, previous_button, next_button, analyze_all_button, play_button, pause_button, stop_button, cancel_analysis_button})});
  auto library_container = Container::Vertical({library_controls, library_menu});
  auto library = Renderer(library_container, [&] {
    Elements rows;
    const auto query = lower(filter);
    library_labels.clear(); library_indices.clear();
    for (int i = 0; i < static_cast<int>(tracks.size()); ++i) {
      if (!query.empty() && lower(tracks[i].title).find(query) == std::string::npos) continue;
      library_labels.push_back(tracks[i].title + "  •  " + tracks[i].path.filename().string()); library_indices.push_back(i);
      if (i == selected) library_visible_selected = static_cast<int>(library_labels.size()) - 1;
    }
    if (library_labels.empty()) rows.push_back(text("No supported audio files in this folder.") | color(Color::GrayLight));
    else rows.push_back(library_menu->Render() | vscroll_indicator | frame);
    const auto name = selected >= 0 ? tracks[selected].title : "No track selected";
    const auto duration = result && result->metadata.sample_rate != 0
        ? std::to_string(result->metadata.frames / result->metadata.sample_rate / 60) + ":" +
            (result->metadata.frames / result->metadata.sample_rate % 60 < 10 ? "0" : "") +
            std::to_string(result->metadata.frames / result->metadata.sample_rate % 60)
        : "—";
    Elements cards = {metric("TEMPO", result ? format_bpm(result->bpm) + " BPM" : "—", Color::GreenLight), metric("BEATS", result ? std::to_string(result->beats) : "—", Color::YellowLight), metric("STATE", result ? beatglass::cli::state_label(result->state) : "waiting", Color::Cyan)};
    const auto progress_total = analysis_total.load(std::memory_order_acquire);
    const auto progress_frames = analysis_frames.load(std::memory_order_acquire);
    const auto progress = progress_total ? std::min<std::uint64_t>(100, progress_frames * 100 / progress_total) : 0;
    const auto completed = batch_completed.load(std::memory_order_acquire);
    const auto total = batch_total.load(std::memory_order_acquire);
    const auto batch_label = total > 1 ? "  [" + std::to_string(completed) + "/" + std::to_string(total) + " tracks]" : "";
    auto status_view = busy ? hbox({spinner(6, static_cast<size_t>(spinner_frame)), text(" "), text(status + batch_label + (progress_total ? "  " + std::to_string(progress) + "%" : ""))}) | color(Color::YellowLight) | bgcolor(Color::RGB(43, 36, 16)) : text(status) | color(Color::GrayLight) | italic;
    const auto playback_label = playback.state() == beatglass::PlaybackState::playing ? "playing" : playback.state() == beatglass::PlaybackState::paused ? "paused" : "stopped";
    const auto playback_status =
#ifdef _WIN32
        playback.status();
#else
        std::string{};
#endif
    std::string playback_position = "—";
    float playback_fraction = -1.0f;
#ifdef _WIN32
    const auto playback_total = playback.total_frames();
    const auto playback_frame = std::min(playback.current_frame(), playback_total);
    if (result && result->metadata.sample_rate != 0 && playback_total != 0) {
      const auto seconds = playback_frame / result->metadata.sample_rate;
      const auto total_seconds = playback_total / result->metadata.sample_rate;
      playback_position = std::to_string(seconds / 60) + ":" + (seconds % 60 < 10 ? "0" : "") + std::to_string(seconds % 60) +
          " / " + std::to_string(total_seconds / 60) + ":" + (total_seconds % 60 < 10 ? "0" : "") + std::to_string(total_seconds % 60);
      playback_fraction = static_cast<float>(playback_frame) / static_cast<float>(playback_total);
    }
#endif
    auto detail = vbox({text("TRACK ANALYSIS") | bold | color(Color::Cyan), text(name) | color(Color::White), selected >= 0 && selected < static_cast<int>(tracks.size()) ? text("Source: " + tracks[selected].path.string()) | color(Color::GrayLight) | italic : text("Source: —") | color(Color::GrayLight) | italic, separator(), hbox(cards) | size(HEIGHT, LESS_THAN, 7), separator(), waveform_chart(result ? waveform : std::vector<float>{}, result ? beat_frames : std::vector<std::uint64_t>{}, result ? result->metadata.frames : 0, playback_fraction), hbox({text("Waveform: ") | color(Color::GrayLight) | bold, text("PCM amplitude") | color(Color::GreenLight), text("  •  "), text("detected beat frames") | color(Color::YellowLight), text("  •  "), text("playback cursor") | color(Color::Cyan)}), separator(), text(result ? "Format: " + result->metadata.format : "Select a track, then press Enter") | bold, text(result ? "Sample rate: " + std::to_string(result->metadata.sample_rate) + " Hz   Channels: " + std::to_string(result->metadata.channels) + "   Duration: " + duration : "Real decoder and aubio analysis will populate this view."), text("Key: " + (key ? *key : "—") + "   Chord: " + (chord ? *chord : "—")) | color(key || chord ? Color::GreenLight : Color::GrayLight), text("Playback: " + std::string(playback_label) + "  position " + playback_position + "  epoch " + std::to_string(playback.epoch())), text(playback_status), separator(), status_view});
    return vbox({text("LIBRARY") | bold | underlined | color(Color::Cyan), separator(), hbox({text("Folder: ") | italic, folder_input->Render() | flex, text(" "), choose_folder_button->Render(), text(" "), open_folder_button->Render(), text(" "), recursive_checkbox->Render(), text(" "), sort_button->Render(), text(" "), refresh_button->Render()}), hbox({text("Filter: ") | italic, filter_input->Render() | flex, text(" "), previous_button->Render(), text(" "), next_button->Render(), text(" "), analyze_all_button->Render(), text(" "), play_button->Render(), text(" "), pause_button->Render(), text(" "), stop_button->Render(), text(" "), cancel_analysis_button->Render(), text(" "), seek_back_button->Render(), text(" "), seek_forward_button->Render()}), separator(), hbox({vbox(rows) | flex, separator(), detail | flex}) | flex, separator(), text("↑↓ / mouse select   Enter analyze   Analyze all visible   Sort title/path   Tab switch tab   q quit") | color(Color::GrayDark) | italic}) | borderRounded | bgcolor(Color::RGB(10, 15, 22)) | flex;
  });
  std::string live_button_label = " Start monitoring ";
  auto toggle_live = [&] {
#ifdef _WIN32
    if (live_session && live_session->state() != beatglass::SourceState::stopped) {
      live_session->stop();
      live_tracking_state = beatglass::TrackingState::stopped;
      live_bpm.reset();
      live_sample_rate = 0; live_observed_frames = 0; live_discontinuities = 0;
      live_status = "Capture stopped.";
      live_button_label = " Start monitoring ";
      return;
    }
    if (live_endpoint_ids.empty()) { live_status = "No render endpoint is available."; return; }
    if (live_selected < 0 || live_selected >= static_cast<int>(live_endpoint_ids.size())) { live_status = "Select an available render endpoint first."; return; }
    if (beatglass::windows::select_render_endpoint(live_endpoint_snapshot, live_endpoint_ids[static_cast<std::size_t>(live_selected)]) != beatglass::windows::SelectionResult::accepted) {
      live_status = "Selected sink is stale; refresh sinks before monitoring.";
      return;
    }
    if (live_session->start(live_endpoint_ids[static_cast<std::size_t>(live_selected)])) {
      live_status = "Starting capture…";
      live_button_label = " Stop monitoring ";
    }
    else live_status = "Capture could not start; inspect endpoint permissions or device state.";
#else
    live_status = "Native loopback capture is unavailable in this build.";
#endif
  };
  auto live_button = Button(&live_button_label, toggle_live);
  auto refresh_live_button = Button(" Refresh sinks ", [&] {
#ifdef _WIN32
    const auto stopped_for_refresh = live_session && live_session->state() != beatglass::SourceState::stopped;
    if (stopped_for_refresh) {
      live_session->stop();
      live_tracking_state = beatglass::TrackingState::stopped;
      live_bpm.reset();
      live_sample_rate = 0; live_observed_frames = 0; live_discontinuities = 0;
      live_button_label = " Start monitoring ";
      live_status = "Capture stopped while render sinks were refreshed.";
    }
    live_endpoint_snapshot = beatglass::windows::enumerate_render_endpoints();
    live_error = live_endpoint_snapshot.error;
    live_endpoints.clear(); live_endpoint_ids.clear(); live_selected = 0;
    for (const auto& endpoint : live_endpoint_snapshot.endpoints) {
      live_endpoints.push_back(endpoint.name + (endpoint.is_default ? " (default)" : ""));
      live_endpoint_ids.push_back(endpoint.id);
      if (endpoint.is_default) live_selected = static_cast<int>(live_endpoint_ids.size()) - 1;
    }
    if (!stopped_for_refresh) live_status = std::to_string(live_endpoints.size()) + " render sinks discovered.";
#else
    live_status = "Native sink discovery is unavailable in this build.";
#endif
  });
  auto reset_live_button = Button(" Reset / reacquire ", [&] {
#ifdef _WIN32
    if (live_session && live_session->state() != beatglass::SourceState::stopped) live_session->stop();
    live_replay = std::make_unique<beatglass::CausalTempoReplay>();
    live_bpm.reset();
    live_tracking_state = beatglass::TrackingState::reacquiring;
    live_sample_rate = 0; live_observed_frames = 0; live_discontinuities = 0;
    live_waveform.assign(live_waveform.size(), 0.0f);
    live_button_label = " Start monitoring ";
    live_status = "Analysis reset; start monitoring to reacquire.";
#else
    live_status = "Native loopback capture is unavailable in this build.";
#endif
  });
  auto set_live_multiplier = [&](float multiplier) {
    live_bpm_multiplier = multiplier;
    live_status = std::string("Displayed tempo correction set to ") + (multiplier < 1.0f ? "half" : multiplier > 1.0f ? "double" : "normal") + " BPM.";
  };
  auto half_bpm_button = Button(" ½ BPM ", [&] { set_live_multiplier(0.5f); });
  auto normal_bpm_button = Button(" Normal BPM ", [&] { set_live_multiplier(1.0f); });
  auto double_bpm_button = Button(" 2× BPM ", [&] { set_live_multiplier(2.0f); });
  RadioboxOption endpoint_picker_option;
  endpoint_picker_option.on_change = [&] {
    if (live_selected < 0 || live_selected >= static_cast<int>(live_endpoints.size())) return;
#ifdef _WIN32
    if (live_session && live_session->state() != beatglass::SourceState::stopped) {
      live_session->stop();
      live_tracking_state = beatglass::TrackingState::stopped;
      live_bpm.reset();
      live_sample_rate = 0; live_observed_frames = 0; live_discontinuities = 0;
      live_button_label = " Start monitoring ";
      live_status = "Capture stopped because the selected sink changed.";
      return;
    }
#endif
    live_status = "Selected sink: " + live_endpoints[static_cast<std::size_t>(live_selected)];
  };
  auto endpoint_picker = Radiobox(&live_endpoints, &live_selected, endpoint_picker_option);
  auto live = Renderer(Container::Vertical({endpoint_picker, Container::Horizontal({live_button, refresh_live_button, reset_live_button, half_bpm_button, normal_bpm_button, double_bpm_button})}), [&] {
    Elements endpoints;
    for (std::size_t index = 0; index < live_endpoints.size(); ++index) {
      auto label = live_endpoints[index];
      endpoints.push_back(text("●  " + label) | color(Color::YellowLight));
    }
    if (endpoints.empty()) endpoints.push_back(text("No active output sinks discovered.") | color(Color::GrayLight));
    const auto live_state = live_tracking_state == beatglass::TrackingState::tracking ? "tracking" : live_tracking_state == beatglass::TrackingState::acquiring ? "acquiring" : live_tracking_state == beatglass::TrackingState::reacquiring ? "reacquiring" : live_tracking_state == beatglass::TrackingState::error ? "error" : "stopped";
    const auto live_starting =
#ifdef _WIN32
        live_session && live_session->state() == beatglass::SourceState::starting;
#else
        false;
#endif
    auto live_status_view = live_starting ? hbox({spinner(6, static_cast<std::size_t>(spinner_frame)), text(" "), text(live_status)}) | color(Color::YellowLight) | bgcolor(Color::RGB(43, 36, 16)) : text(live_status) | color(Color::GrayLight) | italic;
    const auto corrected_bpm = live_bpm ? std::optional<float>(*live_bpm * live_bpm_multiplier) : std::nullopt;
    const auto multiplier_label = live_bpm_multiplier < 1.0f ? "½×" : live_bpm_multiplier > 1.0f ? "2×" : "1×";
    return vbox({text("LIVE") | bold | underlined | color(Color::Cyan), separator(), text("OS AUDIO SINKS") | bold | color(Color::YellowLight), text("Choose a render endpoint before monitoring.") | italic, separator(), endpoint_picker->Render() | frame | vscroll_indicator, live_error.empty() ? text("Endpoint discovery ready.") | color(Color::GreenLight) : text("Endpoint status: " + live_error) | color(Color::YellowLight), separator(), hbox({live_button->Render(), text(" "), refresh_live_button->Render(), text(" "), reset_live_button->Render()}), hbox({text("Tempo interpretation: ") | italic, half_bpm_button->Render(), text(" "), normal_bpm_button->Render(), text(" "), double_bpm_button->Render(), text("  active ") | color(Color::GrayLight), text(multiplier_label) | bold | color(Color::YellowLight)}), live_status_view, separator(), text("LIVE ANALYSIS") | bold | underlined, text("Tempo: " + (corrected_bpm ? format_bpm(corrected_bpm) + " BPM" : "— BPM")) | bold | color(Color::GreenLight), text("Signal: " + (live_sample_rate ? std::to_string(live_sample_rate) + " Hz" : "—") + "   observed " + std::to_string(live_observed_frames) + " frames   discontinuities " + std::to_string(live_discontinuities)), text("Key: —    Chord: —") | italic, text("Tonal analysis: unavailable in the standard build; enable the Essentia music-replay option for Key/Chord.") | color(Color::GrayLight) | italic, text("Capture state: " + std::string(live_state)), separator(), waveform_chart(live_waveform, {}, live_observed_frames), filler(), text("↑↓ choose sink   Tab switch tab   q quit") | color(Color::GrayDark) | italic}) | borderRounded | bgcolor(Color::RGB(10, 15, 22)) | flex;
  });
  auto reduce_motion_checkbox = Checkbox("Reduce motion", &reduce_motion);
  auto settings = Renderer(Container::Vertical({reduce_motion_checkbox, seek_step_input}), [&] {
    return vbox({text("SETTINGS") | bold | underlined | color(Color::Cyan), separator(), text("SESSION") | bold | color(Color::YellowLight),
      text("Library folder: " + root.string()), text("Analysis engine: aubio / file analyzer"),
      text("Tonal engine: optional Essentia music-replay build (disabled in this standard CLI build).") | color(Color::GrayLight) | italic,
      text("Preferences are session-scoped for this prototype."), separator(),
      text("PRESENTATION") | bold | underlined, reduce_motion_checkbox->Render(),
      text(reduce_motion ? "Spinner animation paused; status still updates." : "Spinner animation enabled while work is active.") | color(Color::GrayLight) | italic,
      text("PLAYBACK") | bold | underlined, hbox({text("Seek step (1–60 seconds): ") | italic, seek_step_input->Render() | size(WIDTH, LESS_THAN, 8)}),
      separator(), text("OUTPUT SAFETY") | bold, text("Beatglass never writes tags or modifies source audio."),
      text("Live capture requires an explicit platform source."), filler(), text("Tab switch tab   q quit") | color(Color::GrayDark)}) | borderRounded | flex;
  });
  auto tabs = Container::Tab({library, live, settings}, &tab);
  auto library_tab = Button("  [1] Library  ", [&] { tab = 0; });
  auto live_tab = Button("  [2] Live  ", [&] { tab = 1; });
  auto settings_tab = Button("  [3] Settings  ", [&] { tab = 2; });
  auto tab_bar = Renderer(Container::Horizontal({library_tab, live_tab, settings_tab}), [&] {
    auto library_render = library_tab->Render();
    auto live_render = live_tab->Render();
    auto settings_render = settings_tab->Render();
    const auto active_tab = [](Element element, Color accent) {
      return element | color(Color::White) | bgcolor(accent) | bold | underlined | borderRounded;
    };
    if (tab == 0) library_render = active_tab(library_render, Color::RGB(25, 92, 112));
    else library_render = library_render | color(Color::GrayLight);
    if (tab == 1) live_render = active_tab(live_render, Color::RGB(25, 102, 68));
    else live_render = live_render | color(Color::GrayLight);
    if (tab == 2) settings_render = active_tab(settings_render, Color::RGB(112, 78, 22));
    else settings_render = settings_render | color(Color::GrayLight);
    return hbox({library_render, text(" "), live_render, text(" "), settings_render}) | center;
  });
  auto body = Renderer(Container::Vertical({tab_bar, tabs}), [&]() -> Element {
    const auto compact = !snapshot && (screen.dimx() < 100 || screen.dimy() < 28);
    if (compact) {
      return vbox({tab_bar->Render(), separator(), filler(),
                   text("Beatglass CLI needs more room") | bold | underlined | color(Color::YellowLight) | center,
                   text("Resize the terminal to at least 100 columns × 28 rows.") | italic | color(Color::GrayLight) | center,
                   filler(), separator(), text("Current: " + std::to_string(screen.dimx()) + " × " + std::to_string(screen.dimy())) | color(Color::GrayDark) | center}) |
          bgcolor(Color::RGB(10, 15, 22)) | flex;
    }
    return vbox({tab_bar->Render(), tabs->Render() | flex}) | bgcolor(Color::RGB(10, 15, 22)) | flex;
  });
  auto root_component = CatchEvent(body, [&](Event event) {
    if (event == Event::Character("q") || event == Event::Escape) { screen.Exit(); return true; }
    if (event == Event::Character("1")) { tab = 0; return true; }
    if (event == Event::Character("2")) { tab = 1; return true; }
    if (event == Event::Character("3")) { tab = 2; return true; }
    if (event == Event::Tab) { tab = (tab + 1) % 3; return true; }
    if (event == Event::Character("o") && tab == 0) { open_folder_button->OnEvent(Event::Return); return true; }
    if (tab == 0 && event == Event::Character("[")) { select_relative(-1); return true; }
    if (tab == 0 && event == Event::Character("]")) { select_relative(1); return true; }
    if (event == Event::Return && tab == 0) { analyze(); return true; }
    if (event == Event::Character(" ")) {
      if (tab == 0) {
        if (playback.state() == beatglass::PlaybackState::playing) pause_button->OnEvent(Event::Return);
        else play_button->OnEvent(Event::Return);
      } else if (tab == 1) live_button->OnEvent(Event::Return);
      return true;
    }
    if (event == Event::Custom) {
      if (!reduce_motion) ++spinner_frame;
#ifdef _WIN32
      if (live_session && live_session->state() != beatglass::SourceState::stopped) {
        live_session->poll();
        beatglass::windows::LoopbackPacket packet;
        while (live_session->pop(packet)) {
          const auto layout = packet.channels == 1 ? beatglass::ChannelLayout::mono : packet.channels == 2 ? beatglass::ChannelLayout::stereo : beatglass::ChannelLayout::multichannel;
          beatglass::AudioBlock block{{packet.sample_rate, static_cast<std::uint16_t>(packet.channels), layout}, packet.epoch, packet.first_frame, static_cast<std::int64_t>(packet.qpc_100ns) * 100, packet.discontinuity, std::span<const float>(packet.samples.data(), packet.sample_count)};
          try {
            live_sample_rate = packet.sample_rate;
            live_observed_frames = packet.first_frame + packet.sample_count;
            if (packet.discontinuity) ++live_discontinuities;
            const auto state = live_replay->process(block);
            live_bpm = state.bpm;
            live_tracking_state = state.state;
            for (std::size_t bin = 0; bin < live_waveform.size(); ++bin) {
              const auto first = bin * packet.sample_count / live_waveform.size();
              const auto last = std::max(first + 1, (bin + 1) * packet.sample_count / live_waveform.size());
              float peak = 0.0f;
              for (auto sample = first; sample < std::min<std::size_t>(last, packet.sample_count); ++sample) peak = std::max(peak, std::abs(packet.samples[sample]));
              live_waveform[bin] = peak;
            }
            live_status = "Receiving OS sink audio; analysis is worker-owned and discontinuity-safe.";
          } catch (const std::exception& error) {
            live_tracking_state = beatglass::TrackingState::error;
            live_status = std::string("Live analysis failed: ") + error.what();
            live_session->stop();
          }
        }
      }
      if (live_session && live_session->state() == beatglass::SourceState::stopped) {
        live_button_label = " Start monitoring ";
      }
#endif
      if (busy && analysis_future.valid() && analysis_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        try { auto analysis = analysis_future.get(); for (auto& entry : analysis.results) completed_analyses.insert_or_assign(entry.first, std::move(entry.second)); const auto selection_changed = selected < 0 || selected >= static_cast<int>(tracks.size()) || tracks[selected].path != analysis_path; const auto cached = !selection_changed && completed_analyses.find(analysis_path) != completed_analyses.end(); if (analysis_cancelled.load(std::memory_order_acquire)) { status = "Analysis cancelled; completed track results remain available."; } else if (!cached) { result.reset(); waveform.clear(); beat_frames.clear(); key.reset(); chord.reset(); status = "Analysis finished for a previous track; select a completed track to view its result."; } else { result = completed_analyses.at(analysis_path).result; waveform = completed_analyses.at(analysis_path).waveform; beat_frames = completed_analyses.at(analysis_path).beat_frames; key = completed_analyses.at(analysis_path).key; chord = completed_analyses.at(analysis_path).chord; status = "Analysis complete — decoder PCM drives the waveform and analyzer drives the estimates."; } }
        catch (const std::exception& error) { result.reset(); waveform.clear(); status = std::string("Analysis failed: ") + error.what(); }
        busy = false;
      }
      return true;
    }
    return false;
  });
  if (snapshot) {
    if (argc > 3) {
      const std::string requested_tab(argv[3]);
      if (requested_tab == "library" || requested_tab == "1") tab = 0;
      else if (requested_tab == "live" || requested_tab == "2") tab = 1;
      else if (requested_tab == "settings" || requested_tab == "3") tab = 2;
      else {
        std::cerr << "Invalid snapshot tab: " << requested_tab << " (use library, live, or settings)\n";
        return 2;
      }
    }
    if (tab == 0) { analyze(); if (analysis_future.valid()) { analysis_future.wait(); auto analysis = analysis_future.get(); for (auto& entry : analysis.results) completed_analyses.insert_or_assign(entry.first, std::move(entry.second)); if (const auto cached = completed_analyses.find(analysis_path); cached != completed_analyses.end()) { result = cached->second.result; waveform = cached->second.waveform; beat_frames = cached->second.beat_frames; key = cached->second.key; chord = cached->second.chord; } status = "Analysis complete — decoder PCM drives the waveform and analyzer drives the estimates."; busy = false; } }
    auto rendered = Screen::Create(Dimension::Fixed(120), Dimension::Fixed(36));
    Render(rendered, root_component->Render());
    std::cout << rendered.ToString();
    return 0;
  }
  std::atomic_bool running{true};
  std::thread ticker([&] { while (running.load()) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); screen.PostEvent(Event::Custom); } });
  screen.Loop(root_component);
  running.store(false);
  ticker.join();
  return 0;
}
