#include "beatglass/core/file_analysis.hpp"
#include "beatglass/core/causal_replay.hpp"
#include "beatglass/core/audio.hpp"
#include "beatglass/capture/file_reader.hpp"
#include "beatglass/core/playback_session.hpp"
#include "state_label.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
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
  return vbox({text(std::move(label)) | color(Color::GrayLight), text(std::move(value)) | bold | color(accent)}) | borderRounded | flex;
}
struct AnalysisView {
  beatglass::TempoResult result;
  std::vector<float> waveform;
};
std::vector<float> decode_waveform(const std::filesystem::path& path) {
  beatglass::FileReader reader(path, 1);
  constexpr std::size_t bins = 160;
  std::vector<float> peaks(bins, 0.0f);
  std::vector<float> buffer(4096 * 2);
  std::uint64_t frames_read = 0;
  const auto total_frames = reader.metadata().frames;
  for (;;) {
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
Element waveform_chart(const std::vector<float>& waveform, std::uint64_t beats) {
  return vbox({text("WAVEFORM") | bold | color(Color::GrayLight),
               graph([=](int width, int height) {
                 std::vector<int> values(width);
                 for (int x = 0; x < width; ++x) {
                   const auto sample = waveform.empty() ? 0.08f : waveform[std::min<std::size_t>(waveform.size() - 1, static_cast<std::size_t>(x * waveform.size() / std::max(1, width)))];
                   const auto beat_marker = !waveform.empty() && beats > 0 && x % std::max(1, width / static_cast<int>(std::min<std::uint64_t>(beats, 16))) == 0 ? 0.12f : 0.0f;
                   values[x] = std::clamp(static_cast<int>((sample + beat_marker) * height), 0, height - 1);
                 }
                 return values;
               }) | color(Color::GreenLight) | borderRounded}) | flex;
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
  void stop() { request(Command::stop); }
  beatglass::PlaybackState state() const noexcept { return state_.load(std::memory_order_acquire); }
  std::uint64_t epoch() const noexcept { return epoch_.load(std::memory_order_acquire); }
  std::string status() const { std::lock_guard lock(mutex_); return status_; }
private:
  enum class Command { none, play, pause, stop, shutdown };
  void request(Command command) { { std::lock_guard lock(mutex_); command_ = command; } condition_.notify_one(); }
  Command take_command() { std::lock_guard lock(mutex_); const auto command = command_; command_ = Command::none; return command; }
  void set_status(std::string status) { std::lock_guard lock(mutex_); status_ = std::move(status); }
  void run() {
    std::unique_ptr<beatglass::FileReader> reader;
    beatglass::windows::WindowsPlaybackSession output;
    std::vector<float> decoded(4096 * 2), render(4096 * 2);
    std::uint64_t next_epoch = 0;
    bool shutdown = false;
    while (!shutdown) {
      auto command = take_command();
      if (command == Command::shutdown) break;
      if (command == Command::play) {
        output.stop(); reader.reset(); state_.store(beatglass::PlaybackState::stopped, std::memory_order_release);
        try {
          reader = std::make_unique<beatglass::FileReader>(path_, ++next_epoch);
          std::string error;
          const auto& metadata = reader->metadata();
          if (!output.start(metadata.sample_rate, static_cast<std::uint16_t>(metadata.channels), decoded.size(), next_epoch, error)) { set_status("Playback unavailable: " + error); reader.reset(); continue; }
          epoch_.store(next_epoch, std::memory_order_release); state_.store(beatglass::PlaybackState::playing, std::memory_order_release); set_status("Playing decoded PCM through native output.");
        } catch (const std::exception& error) { set_status(std::string("Playback failed: ") + error.what()); reader.reset(); continue; }
      } else if (command == Command::pause) {
        if (output.pause()) { state_.store(beatglass::PlaybackState::paused, std::memory_order_release); set_status("Playback paused."); }
      } else if (command == Command::stop) {
        output.stop(); reader.reset(); state_.store(beatglass::PlaybackState::stopped, std::memory_order_release); set_status("Playback stopped.");
      }
      if (!reader || state_.load(std::memory_order_acquire) != beatglass::PlaybackState::playing) {
        std::unique_lock lock(mutex_); condition_.wait_for(lock, std::chrono::milliseconds(20), [&] { return command_ != Command::none; }); continue;
      }
      const auto read = reader->read(decoded);
      if (read.status == beatglass::FileReadStatus::end) { output.stop(); reader.reset(); state_.store(beatglass::PlaybackState::stopped, std::memory_order_release); set_status("Playback reached end of file."); continue; }
      if (read.status != beatglass::FileReadStatus::data) continue;
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
};
#endif
}  // namespace

int main(int argc, char** argv) {
  using namespace ftxui;
  const bool snapshot = argc > 1 && std::string(argv[1]) == "--snapshot";
  std::filesystem::path root = snapshot ? (argc > 2 ? argv[2] : std::filesystem::current_path()) : (argc > 1 ? argv[1] : std::filesystem::current_path());
  auto tracks = discover(root, true);
  std::string folder_text = root.string();
  std::string filter;
  bool recursive = true;
  std::vector<std::string> live_endpoints;
  std::vector<std::string> live_endpoint_ids;
  std::string live_error;
#ifdef _WIN32
  const auto endpoint_snapshot = beatglass::windows::enumerate_render_endpoints();
  live_error = endpoint_snapshot.error;
  for (const auto& endpoint : endpoint_snapshot.endpoints) {
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
  for (int index = 0; index < static_cast<int>(endpoint_snapshot.endpoints.size()); ++index) if (endpoint_snapshot.endpoints[index].is_default) live_selected = index;
#endif
  std::string status = tracks.empty() ? "Choose a folder with audio files to begin." : "Ready to analyze a track.";
  std::optional<beatglass::TempoResult> result;
  std::vector<float> waveform;
  bool busy = false;
  bool reduce_motion = false;
#ifdef _WIN32
  CliPlayback playback;
#else
  beatglass::PlaybackSession playback;
#endif
  std::uint64_t playback_epoch = 0;
  int spinner_frame = 0;
  std::future<AnalysisView> analysis_future;
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
  std::vector<float> live_waveform(160, 0.0f);
  live_session = std::make_unique<beatglass::windows::LoopbackSession>(live_epochs, [&] {
    live_replay = std::make_unique<beatglass::CausalTempoReplay>();
    live_bpm.reset();
    live_tracking_state = beatglass::TrackingState::reacquiring;
  });
#else
  std::string live_status = "Native loopback capture is unavailable in this build.";
  std::optional<float> live_bpm;
  beatglass::TrackingState live_tracking_state = beatglass::TrackingState::stopped;
  std::vector<float> live_waveform(160, 0.0f);
#endif
  auto screen = ScreenInteractive::TerminalOutput();
  auto analyze = [&] {
    if (selected < 0 || selected >= static_cast<int>(tracks.size()) || busy) return;
    busy = true; status = "Analyzing " + tracks[selected].title + "…";
    const auto path = tracks[selected].path;
    analysis_future = std::async(std::launch::async, [path] { return AnalysisView{beatglass::analyze_file(path), decode_waveform(path)}; });
  };
  auto refresh = [&] { tracks = discover(root, recursive); selected = tracks.empty() ? -1 : std::min(selected, static_cast<int>(tracks.size()) - 1); result.reset(); waveform.clear(); status = std::to_string(tracks.size()) + " tracks indexed."; };
  auto filter_input = Input(&filter, "Filter songs");
  auto folder_input = Input(&folder_text, "Audio folder");
  auto recursive_checkbox = Checkbox("Recursive", &recursive);
  auto refresh_button = Button(" Refresh ", refresh);
  auto open_folder_button = Button(" Open folder ", [&] {
    const std::filesystem::path requested(folder_text);
    std::error_code error;
    if (!std::filesystem::is_directory(requested, error)) { status = "Folder unavailable: " + folder_text; return; }
    root = requested; refresh(); status = "Opened " + root.string() + " — " + std::to_string(tracks.size()) + " tracks indexed.";
  });
  auto play_button = Button(" Play ", [&] {
    if (selected >= 0 && selected < static_cast<int>(tracks.size())) {
#ifdef _WIN32
      playback.play(tracks[selected].path);
#else
      ++playback_epoch;
      playback.start(playback_epoch);
#endif
      status = "Playing " + tracks[selected].title + " through the selected output.";
    }
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
  MenuOption library_menu_option;
  library_menu_option.on_change = [&] {
    if (library_visible_selected >= 0 && library_visible_selected < static_cast<int>(library_indices.size())) selected = library_indices[static_cast<std::size_t>(library_visible_selected)];
  };
  auto library_menu = Menu(&library_labels, &library_visible_selected, library_menu_option);
  auto library_controls = Container::Vertical({Container::Horizontal({folder_input, open_folder_button, recursive_checkbox, refresh_button}), Container::Horizontal({filter_input, play_button, pause_button, stop_button})});
  auto library_container = Container::Vertical({library_controls, library_menu});
  auto library = Renderer(library_container, [&] {
    Elements rows;
    const auto query = lower(filter);
    library_labels.clear(); library_indices.clear();
    for (int i = 0; i < static_cast<int>(tracks.size()); ++i) {
      if (!query.empty() && lower(tracks[i].title).find(query) == std::string::npos) continue;
      library_labels.push_back(tracks[i].title); library_indices.push_back(i);
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
    auto status_view = busy ? hbox({spinner(6, static_cast<size_t>(spinner_frame)), text(" "), text(status)}) | color(Color::YellowLight) : text(status) | color(Color::GrayLight);
    const auto playback_label = playback.state() == beatglass::PlaybackState::playing ? "playing" : playback.state() == beatglass::PlaybackState::paused ? "paused" : "stopped";
    const auto playback_status =
#ifdef _WIN32
        playback.status();
#else
        std::string{};
#endif
    auto detail = vbox({text("TRACK ANALYSIS") | bold | color(Color::Cyan), text(name) | color(Color::White), separator(), hbox(cards) | size(HEIGHT, LESS_THAN, 7), separator(), waveform_chart(result ? waveform : std::vector<float>{}, result ? result->beats : 0), separator(), text(result ? "Format: " + result->metadata.format : "Select a track, then press Enter") | bold, text(result ? "Sample rate: " + std::to_string(result->metadata.sample_rate) + " Hz   Channels: " + std::to_string(result->metadata.channels) + "   Duration: " + duration : "Real decoder and aubio analysis will populate this view."), text("Playback: " + std::string(playback_label) + "  epoch " + std::to_string(playback.epoch())), text(playback_status), separator(), status_view});
    return vbox({text("LIBRARY") | bold | color(Color::Cyan), separator(), hbox({text("Folder: "), folder_input->Render() | flex, text(" "), open_folder_button->Render(), text(" "), recursive_checkbox->Render(), text(" "), refresh_button->Render()}), hbox({text("Filter: "), filter_input->Render() | flex, text(" "), play_button->Render(), text(" "), pause_button->Render(), text(" "), stop_button->Render()}), separator(), hbox({vbox(rows) | flex, separator(), detail | flex}) | flex, separator(), text("↑↓ / mouse select   Enter analyze   Tab switch tab   q quit") | color(Color::GrayDark)}) | borderRounded | flex;
  });
  auto toggle_live = [&] {
#ifdef _WIN32
    if (live_session && live_session->state() != beatglass::SourceState::stopped) {
      live_session->stop();
      live_tracking_state = beatglass::TrackingState::stopped;
      live_bpm.reset();
      live_status = "Capture stopped.";
      return;
    }
    if (live_endpoint_ids.empty()) { live_status = "No render endpoint is available."; return; }
    if (live_selected < 0 || live_selected >= static_cast<int>(live_endpoint_ids.size())) { live_status = "Select an available render endpoint first."; return; }
    if (live_session->start(live_endpoint_ids[static_cast<std::size_t>(live_selected)])) live_status = "Starting capture…";
    else live_status = "Capture could not start; inspect endpoint permissions or device state.";
#else
    live_status = "Native loopback capture is unavailable in this build.";
#endif
  };
  auto live_button = Button(" Start monitoring ", toggle_live);
  auto refresh_live_button = Button(" Refresh sinks ", [&] {
#ifdef _WIN32
    const auto refreshed = beatglass::windows::enumerate_render_endpoints();
    live_error = refreshed.error;
    live_endpoints.clear(); live_endpoint_ids.clear(); live_selected = 0;
    for (const auto& endpoint : refreshed.endpoints) {
      live_endpoints.push_back(endpoint.name + (endpoint.is_default ? " (default)" : ""));
      live_endpoint_ids.push_back(endpoint.id);
      if (endpoint.is_default) live_selected = static_cast<int>(live_endpoint_ids.size()) - 1;
    }
    live_status = std::to_string(live_endpoints.size()) + " render sinks discovered.";
#else
    live_status = "Native sink discovery is unavailable in this build.";
#endif
  });
  auto endpoint_picker = Radiobox(&live_endpoints, &live_selected);
  auto live = Renderer(Container::Vertical({endpoint_picker, Container::Horizontal({live_button, refresh_live_button})}), [&] {
    Elements endpoints;
    for (std::size_t index = 0; index < live_endpoints.size(); ++index) {
      auto label = live_endpoints[index];
      endpoints.push_back(text("●  " + label) | color(Color::YellowLight));
    }
    if (endpoints.empty()) endpoints.push_back(text("No active output sinks discovered.") | color(Color::GrayLight));
    const auto live_state = live_tracking_state == beatglass::TrackingState::tracking ? "tracking" : live_tracking_state == beatglass::TrackingState::acquiring ? "acquiring" : live_tracking_state == beatglass::TrackingState::reacquiring ? "reacquiring" : live_tracking_state == beatglass::TrackingState::error ? "error" : "stopped";
    return vbox({text("LIVE") | bold | color(Color::Cyan), separator(), text("OS AUDIO SINKS") | bold, text("Choose a render endpoint before monitoring."), separator(), endpoint_picker->Render() | frame | vscroll_indicator, live_error.empty() ? text("Endpoint discovery ready.") | color(Color::GreenLight) : text("Endpoint status: " + live_error) | color(Color::YellowLight), separator(), hbox({live_button->Render(), text(" "), refresh_live_button->Render()}), text(live_status) | color(Color::GrayLight), separator(), text("LIVE ANALYSIS") | bold, text("Tempo: " + (live_bpm ? format_bpm(live_bpm) + " BPM" : "— BPM")), text("Key: —    Chord: —"), text("Capture state: " + std::string(live_state)), separator(), waveform_chart(live_waveform, 0), filler(), text("↑↓ choose sink   Tab switch tab   q quit") | color(Color::GrayDark)}) | borderRounded | flex;
  });
  auto reduce_motion_checkbox = Checkbox("Reduce motion", &reduce_motion);
  auto settings = Renderer(Container::Vertical({reduce_motion_checkbox}), [&] {
    return vbox({text("SETTINGS") | bold | color(Color::Cyan), separator(), text("SESSION") | bold,
      text("Library folder: " + root.string()), text("Analysis engine: aubio / file analyzer"),
      text("Preferences are session-scoped for this prototype."), separator(),
      text("PRESENTATION") | bold, reduce_motion_checkbox->Render(),
      text(reduce_motion ? "Spinner animation paused; status still updates." : "Spinner animation enabled while work is active.") | color(Color::GrayLight),
      separator(), text("OUTPUT SAFETY") | bold, text("Beatglass never writes tags or modifies source audio."),
      text("Live capture requires an explicit platform source."), filler(), text("Tab switch tab   q quit") | color(Color::GrayDark)}) | borderRounded | flex;
  });
  auto tabs = Container::Tab({library, live, settings}, &tab);
  auto library_tab = Button("  Library  ", [&] { tab = 0; });
  auto live_tab = Button("  Live  ", [&] { tab = 1; });
  auto settings_tab = Button("  Settings  ", [&] { tab = 2; });
  auto tab_bar = Renderer(Container::Horizontal({library_tab, live_tab, settings_tab}), [&] {
    auto library_render = library_tab->Render();
    auto live_render = live_tab->Render();
    auto settings_render = settings_tab->Render();
    if (tab == 0) library_render = library_render | color(Color::Cyan) | bold;
    if (tab == 1) live_render = live_render | color(Color::Cyan) | bold;
    if (tab == 2) settings_render = settings_render | color(Color::Cyan) | bold;
    return hbox({library_render, text(" "), live_render, text(" "), settings_render}) | center;
  });
  auto root_component = CatchEvent(Container::Vertical({tab_bar, tabs}), [&](Event event) {
    if (event == Event::Character("q") || event == Event::Escape) { screen.Exit(); return true; }
    if (event == Event::Character("1")) { tab = 0; return true; }
    if (event == Event::Character("2")) { tab = 1; return true; }
    if (event == Event::Character("3")) { tab = 2; return true; }
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
#endif
      if (busy && analysis_future.valid() && analysis_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        try { auto analysis = analysis_future.get(); result = std::move(analysis.result); waveform = std::move(analysis.waveform); status = "Analysis complete — decoder PCM drives the waveform and analyzer drives the estimates."; }
        catch (const std::exception& error) { result.reset(); waveform.clear(); status = std::string("Analysis failed: ") + error.what(); }
        busy = false;
      }
      return true;
    }
    return false;
  });
  if (snapshot) {
    if (argc > 3) tab = std::clamp(std::stoi(argv[3]), 0, 2);
    if (tab == 0) { analyze(); if (analysis_future.valid()) { analysis_future.wait(); auto analysis = analysis_future.get(); result = std::move(analysis.result); waveform = std::move(analysis.waveform); status = "Analysis complete — decoder PCM drives the waveform and analyzer drives the estimates."; busy = false; } }
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
