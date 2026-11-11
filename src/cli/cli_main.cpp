#include "beatglass/core/file_analysis.hpp"
#include "beatglass/core/causal_replay.hpp"
#include "beatglass/core/audio.hpp"
#include "beatglass/core/presentation.hpp"
#include "beatglass/core/visualization.hpp"
#include "beatglass/core/visualization_presentation.hpp"
#include "beatglass/capture/file_reader.hpp"
#include "beatglass/core/playback_session.hpp"
#if defined(BEATGLASS_HAS_PIPEWIRE)
#include "beatglass/capture/pipewire_source.hpp"
#if !defined(_WIN32) && !defined(__APPLE__)
#include "beatglass/desktop/pipewire_playback_session.hpp"
#endif
#endif
#ifdef BEATGLASS_CLI_MUSIC_REPLAY
#include "beatglass/core/music_replay.hpp"
#endif

#if defined(BEATGLASS_HAS_PIPEWIRE) && !defined(_WIN32) && !defined(__APPLE__)
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>
#endif

#include <array>
#include <cstdlib>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#if defined(BEATGLASS_HAS_PIPEWIRE) && !defined(_WIN32) && !defined(__APPLE__)
class CliPlayback final {
public:
  CliPlayback() : worker_([this] { run(); }) {}
  ~CliPlayback() { { std::lock_guard lock(mutex_); command_ = Command::shutdown; } condition_.notify_one(); worker_.join(); }
  void play(const std::filesystem::path& path) { { std::lock_guard lock(mutex_); path_ = path; command_ = Command::play; } condition_.notify_one(); }
  void resume() { request(Command::resume); }
  void pause() { request(Command::pause); }
  void seek_seconds(std::int64_t seconds) { { std::lock_guard lock(mutex_); seek_delta_seconds_ = seconds; command_ = Command::seek; } condition_.notify_one(); }
  void stop() { request(Command::stop); }
  beatglass::PlaybackState state() const noexcept { return state_.load(std::memory_order_acquire); }
  std::uint64_t epoch() const noexcept { return epoch_.load(std::memory_order_acquire); }
  std::uint64_t current_frame() const noexcept { return current_frame_.load(std::memory_order_acquire); }
  std::uint64_t total_frames() const noexcept { return total_frames_.load(std::memory_order_acquire); }
  std::string status() const { std::lock_guard lock(mutex_); return status_; }
private:
  enum class Command { none, play, resume, pause, seek, stop, shutdown };
  void request(Command command) { { std::lock_guard lock(mutex_); command_ = command; } condition_.notify_one(); }
  Command take_command() { std::lock_guard lock(mutex_); const auto command = command_; command_ = Command::none; return command; }
  std::int64_t take_seek_delta_seconds() { std::lock_guard lock(mutex_); const auto delta = seek_delta_seconds_; seek_delta_seconds_ = 0; return delta; }
  void set_status(std::string status) { std::lock_guard lock(mutex_); status_ = std::move(status); }
  void run() {
    std::unique_ptr<beatglass::FileReader> reader;
    beatglass::pipewire::PlaybackSession output;
    std::vector<float> decoded(4096);
    std::uint64_t next_epoch = 0;
    bool shutdown = false;
    while (!shutdown) {
      const auto command = take_command();
      if (command == Command::shutdown) break;
      if (command == Command::play) {
        output.stop(); reader.reset(); state_.store(beatglass::PlaybackState::stopped, std::memory_order_release);
        session_.stop();
        try {
          reader = std::make_unique<beatglass::FileReader>(path_, ++next_epoch);
          const auto& metadata = reader->metadata(); std::string error;
          current_frame_.store(0, std::memory_order_release); total_frames_.store(metadata.frames, std::memory_order_release);
          if (!output.start(metadata.sample_rate, static_cast<std::uint16_t>(metadata.channels), decoded.size(), next_epoch, error)) { set_status("Playback unavailable: " + error); reader.reset(); continue; }
          if (!session_.start(next_epoch)) { output.stop(); reader.reset(); set_status("Playback session could not acquire a fresh epoch."); continue; }
          epoch_.store(next_epoch, std::memory_order_release); state_.store(beatglass::PlaybackState::playing, std::memory_order_release); set_status(output.status());
        } catch (const std::exception& error) { set_status(std::string("Playback failed: ") + error.what()); reader.reset(); continue; }
      } else if (command == Command::resume) {
        if (reader && output.resume() && session_.resume()) { state_.store(beatglass::PlaybackState::playing, std::memory_order_release); set_status(output.status()); }
      } else if (command == Command::pause) {
        if (reader && output.pause() && session_.pause()) { state_.store(beatglass::PlaybackState::paused, std::memory_order_release); set_status(output.status()); }
      } else if (command == Command::seek && reader) {
        try {
          const auto& metadata = reader->metadata(); const auto delta = take_seek_delta_seconds() * static_cast<std::int64_t>(metadata.sample_rate); const auto current = current_frame_.load(std::memory_order_acquire);
          const auto target = delta < 0 && current < static_cast<std::uint64_t>(-delta) ? 0 : delta < 0 ? current - static_cast<std::uint64_t>(-delta) : std::min<std::uint64_t>(metadata.frames, current + static_cast<std::uint64_t>(delta));
          output.stop(); reader->seek(target, ++next_epoch); std::string error;
          if (!output.start(metadata.sample_rate, static_cast<std::uint16_t>(metadata.channels), decoded.size(), next_epoch, error)) throw std::runtime_error(error);
          if (!session_.seek(target, next_epoch)) throw std::runtime_error("Playback session rejected the seek epoch");
          current_frame_.store(target, std::memory_order_release); epoch_.store(next_epoch, std::memory_order_release); state_.store(beatglass::PlaybackState::playing, std::memory_order_release); set_status("Playback seeked to frame " + std::to_string(target) + ".");
        } catch (const std::exception& error) { set_status(std::string("Playback seek failed: ") + error.what()); }
      } else if (command == Command::stop) {
        output.stop(); session_.stop(); reader.reset(); current_frame_.store(0, std::memory_order_release); total_frames_.store(0, std::memory_order_release); state_.store(beatglass::PlaybackState::stopped, std::memory_order_release); set_status(output.status());
      }
      if (!reader || state_.load(std::memory_order_acquire) != beatglass::PlaybackState::playing) { std::unique_lock lock(mutex_); condition_.wait_for(lock, std::chrono::milliseconds(20), [&] { return command_ != Command::none; }); continue; }
      const auto read = reader->read(decoded);
      if (read.status == beatglass::FileReadStatus::end) { output.stop(); session_.stop(); reader.reset(); state_.store(beatglass::PlaybackState::stopped, std::memory_order_release); set_status("Playback reached end of file."); continue; }
      if (read.status != beatglass::FileReadStatus::data) continue;
      while (!output.submit(read.block)) {
        const auto pending = take_command();
        if (pending == Command::stop || pending == Command::shutdown) { output.stop(); session_.stop(); reader.reset(); state_.store(beatglass::PlaybackState::stopped, std::memory_order_release); shutdown = pending == Command::shutdown; break; }
        if (pending == Command::pause) { output.pause(); session_.pause(); state_.store(beatglass::PlaybackState::paused, std::memory_order_release); set_status(output.status()); break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
      }
      if (reader && state_.load(std::memory_order_acquire) == beatglass::PlaybackState::playing) {
        const auto frame_count = read.block.samples.size() / std::max<std::size_t>(1, read.block.format.channels);
        if (!session_.accept(read.block.epoch, read.block.first_frame, frame_count)) {
          output.stop(); session_.stop(); reader.reset(); state_.store(beatglass::PlaybackState::stopped, std::memory_order_release);
          set_status("Playback stopped after an out-of-order decoded block.");
          continue;
        }
        current_frame_.store(session_.next_frame(), std::memory_order_release);
      }
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
  std::atomic<std::uint64_t> epoch_{}, current_frame_{}, total_frames_{};
  std::int64_t seek_delta_seconds_{};
  beatglass::PlaybackSession session_;
};
#endif

#if !defined(_WIN32) && !defined(__APPLE__) && !defined(BEATGLASS_HAS_PIPEWIRE)
#include <filesystem>
#include <string>
class CliPlayback final {
public:
  void play(const std::filesystem::path&) noexcept {}
  void resume() noexcept {}
  void pause() noexcept {}
  void seek_seconds(std::int64_t) noexcept {}
  void stop() noexcept {}
  beatglass::PlaybackState state() const noexcept { return beatglass::PlaybackState::stopped; }
  std::uint64_t epoch() const noexcept { return 0; }
  std::uint64_t current_frame() const noexcept { return 0; }
  std::uint64_t total_frames() const noexcept { return 0; }
  std::string status() const { return "Native playback is unavailable in this build."; }
};
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
#include "../desktop/platform/windows_waveout_devices.hpp"
#endif
#ifdef __APPLE__
#include "beatglass/desktop/macos_playback_session.hpp"
#include "beatglass/capture/coreaudio_source_internal.hpp"
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
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <memory>
#include <map>
#include <set>
#include <optional>
#include <sstream>
#include <string>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <vector>
#ifdef BEATGLASS_CLI_LIBRARY
#include "beatglass/library/storage_owner.hpp"
#include "beatglass/library/analysis_operation.hpp"
#include "beatglass/library/file_analysis_operation.hpp"
#endif
#include <mutex>

namespace {

#ifdef __APPLE__
class MacMonitorBackend final {
public:
  struct Status {
    bool active{};
    bool needs_restart{};
    bool overflowed{};
    bool error{};
    std::string message;
    bool has_error() const noexcept { return error; }
  };
  MacMonitorBackend() : source_(beatglass::coreaudio::make_native_backend()) {}
  bool start_stream(const std::string& id, std::uint64_t epoch, std::uint32_t, std::uint32_t) { selected_ = id; epoch_ = epoch; return source_ && source_->start(id, epoch); }
  bool reacquire(const std::string& id, std::uint64_t epoch, std::uint32_t, std::uint32_t) {
    if (!source_ || id.empty() || !epoch || epoch == epoch_) return false;
    source_->stop();
    selected_ = id;
    epoch_ = epoch;
    return source_->start(id, epoch);
  }
  bool selected_active() const { return source_ && source_->status().state != beatglass::coreaudio::TapState::stopped; }
  void stop() noexcept { if (source_) source_->stop(); }
  bool pop_current(beatglass::coreaudio::CaptureBlock& block) noexcept { return source_ && source_->pop(block) && block.epoch == epoch_; }
  Status poll_status() { Status out; if (!source_) { out.error = true; out.message = "CoreAudio source is unavailable."; return out; } const auto status = source_->status(); out.active = status.state == beatglass::coreaudio::TapState::running; out.needs_restart = status.state == beatglass::coreaudio::TapState::interrupted; out.error = status.state == beatglass::coreaudio::TapState::error; out.message = status.message; return out; }
  beatglass::EnumerationResult enumerate() const { return source_ ? source_->enumerate() : beatglass::EnumerationResult{}; }
private:
  std::unique_ptr<beatglass::coreaudio::SourceBackend> source_;
  std::string selected_;
  std::uint64_t epoch_{};
};
#endif

struct CliPreferences {
  bool reduce_motion{};
  int seek_step{5};
  std::string visual_mode{"combined"};
  int playback_device_id{-1};
  int bpm_multiplier{0};
  std::string live_endpoint_id;
  std::string live_endpoint_name;
  std::string library_folder;
  std::vector<std::string> library_folders;
};

std::filesystem::path preferences_path() {
#ifdef _WIN32
  if (const auto* app_data = std::getenv("APPDATA"); app_data && *app_data)
    return std::filesystem::path(app_data) / "Beatglass" / "beatglass-cli.settings";
#else
  if (const auto* config_home = std::getenv("XDG_CONFIG_HOME"); config_home && *config_home)
    return std::filesystem::path(config_home) / "beatglass" / "settings";
  if (const auto* home = std::getenv("HOME"); home && *home)
    return std::filesystem::path(home) / ".config" / "beatglass" / "settings";
#endif
  return {};
}

CliPreferences load_preferences() {
  CliPreferences preferences;
  const auto path = preferences_path();
  if (path.empty()) return preferences;
  std::ifstream input(path);
  std::string line;
  while (std::getline(input, line)) {
    std::istringstream fields(line);
    std::string key, value;
    if (!(fields >> key)) continue;
    if (key == "library_folder") {
      fields >> std::quoted(preferences.library_folder);
      continue;
    }
    if (key == "library_folder_entry") {
      std::string folder;
      fields >> std::quoted(folder);
      if (!folder.empty()) preferences.library_folders.push_back(std::move(folder));
      continue;
    }
    if (key == "live_endpoint_id") {
      fields >> std::quoted(preferences.live_endpoint_id);
      continue;
    }
    if (key == "live_endpoint_name") {
      fields >> std::quoted(preferences.live_endpoint_name);
      continue;
    }
    if (!(fields >> value)) continue;
    if (key == "reduce_motion") preferences.reduce_motion = value == "1";
    else if (key == "seek_step") {
      try { preferences.seek_step = std::clamp(std::stoi(value), 1, 60); } catch (...) {}
    }
    else if (key == "visual_mode" && (value == "waveform" || value == "spectrum" || value == "combined" || value == "beat_grid" || value == "tonal_history")) preferences.visual_mode = value;
    else if (key == "playback_device_id") { try { preferences.playback_device_id = std::stoi(value); } catch (...) {} }
    else if (key == "bpm_multiplier") { try { preferences.bpm_multiplier = std::clamp(std::stoi(value), -1, 1); } catch (...) {} }
  }
  return preferences;
}

void save_preferences(const CliPreferences& preferences) {
  const auto path = preferences_path();
  if (path.empty()) return;
  std::error_code error;
  std::filesystem::create_directories(path.parent_path(), error);
  if (error) return;
  std::ofstream output(path, std::ios::trunc);
  if (output) {
    output << "reduce_motion " << (preferences.reduce_motion ? 1 : 0) << "\nseek_step " << preferences.seek_step << "\nvisual_mode " << preferences.visual_mode << "\nplayback_device_id " << preferences.playback_device_id << "\nbpm_multiplier " << preferences.bpm_multiplier << "\nlive_endpoint_id " << std::quoted(preferences.live_endpoint_id) << "\nlive_endpoint_name " << std::quoted(preferences.live_endpoint_name) << "\nlibrary_folder " << std::quoted(preferences.library_folder) << "\n";
    for (const auto& folder : preferences.library_folders) output << "library_folder_entry " << std::quoted(folder) << "\n";
  }
}
using namespace ftxui;
// Apple SDK MacTypes.h exports global enum constants named bold and italic.
// Rewrite the compact styling tokens only after all Apple headers are included;
// the macros are confined to this executable translation unit.
#ifdef __APPLE__
#define bold ftxui::bold
#define italic ftxui::italic
#endif

std::string shell_quote(const std::string& value) {
  std::string quoted = "'";
  for (const char character : value) {
    if (character == '\'') quoted += "'\\''";
    else quoted += character;
  }
  quoted += "'";
  return quoted;
}

bool open_folder_in_file_manager(const std::filesystem::path& folder, std::string& error) {
  if (folder.empty() || !std::filesystem::is_directory(folder)) {
    error = "Folder unavailable: " + folder.string();
    return false;
  }
#ifdef _WIN32
  const auto result = reinterpret_cast<std::intptr_t>(ShellExecuteW(nullptr, L"open", folder.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL));
  if (result <= 32) {
    error = "Windows could not open the folder (ShellExecute error " + std::to_string(result) + ").";
    return false;
  }
#elif defined(__APPLE__)
  const auto command = "open -- " + shell_quote(folder.string());
  if (std::system(command.c_str()) != 0) {
    error = "macOS could not open the folder.";
    return false;
  }
#else
  const auto command = "xdg-open -- " + shell_quote(folder.string());
  if (std::system(command.c_str()) != 0) {
    error = "The desktop could not open the folder (xdg-open failed).";
    return false;
  }
#endif
  return true;
}


struct Track { std::filesystem::path path; std::string title; std::optional<beatglass::FileMetadata> metadata; };
std::string format_bpm(const std::optional<float>& bpm) {
  if (!bpm) return "—";
  std::ostringstream out; out << std::fixed << std::setprecision(1) << *bpm; return out.str();
}
std::string format_duration(const beatglass::FileMetadata& metadata) {
  if (!metadata.sample_rate) return "—";
  const auto seconds = metadata.frames / metadata.sample_rate;
  return std::to_string(seconds / 60) + ":" + (seconds % 60 < 10 ? "0" : "") + std::to_string(seconds % 60);
}
Element metric(std::string label, std::string value, Color accent) {
  return vbox({text(std::move(label)) | color(Color::GrayLight) | italic,
               text(std::move(value)) | bold | color(accent)}) | borderRounded | bgcolor(Color::RGB(18, 24, 31)) | flex;
}
Element action(Element element, Color accent) {
  return element | color(accent) | bold;
}
Element progress_meter(std::uint64_t percent) {
  constexpr std::size_t width = 36;
  const auto filled = std::min<std::size_t>(width, static_cast<std::size_t>(percent * width / 100));
  return hbox({text("["), text(std::string(filled, '#')) | color(Color::GreenLight) | bold,
               text(std::string(width - filled, '-')) | color(Color::GrayDark), text("] ") ,
               text(std::to_string(percent) + "%") | color(Color::YellowLight) | bold});
}
struct AnalysisView {
  beatglass::TempoResult result;
  std::vector<float> waveform;
  std::vector<std::uint64_t> beat_frames;
  std::optional<std::string> key;
  std::optional<std::string> chord;
  std::vector<std::string> tonal_history;
  bool from_persistent_cache{};
};
beatglass::PresentationState present_analysis_view(const AnalysisView& view) {
  beatglass::Snapshot snapshot;
  snapshot.state = view.result.state;
  snapshot.observed_through = {1, view.result.metadata.frames};
  snapshot.bpm = view.result.bpm;
  for (const auto frame : view.beat_frames) snapshot.beats.push_back({{1, frame}, 1.0f});
  if (view.key) snapshot.key_candidates.push_back({*view.key, 0.0f});
  for (const auto& label : view.tonal_history) {
    if (!label.empty()) snapshot.chords.push_back({{1, 0}, {1, view.result.metadata.frames}, {label, 0.0f}});
  }
  return beatglass::present_snapshot(snapshot);
}
struct AnalysisBatch {
  std::vector<std::pair<std::filesystem::path, AnalysisView>> results;
  std::vector<std::pair<std::filesystem::path, std::string>> errors;
  std::size_t cache_hits{};
};
#ifdef BEATGLASS_CLI_LIBRARY
struct CacheSource {
  beatglass::FileVersion source;
  beatglass::AnalysisIdentity identity;
  std::string run_id;
};
CacheSource cache_source(const std::filesystem::path& path) {
  std::error_code error;
  const auto size = std::filesystem::file_size(path, error);
  if (error) throw std::runtime_error("cache identity size failed: " + error.message());
  const auto modified = static_cast<std::int64_t>(std::filesystem::last_write_time(path, error).time_since_epoch().count());
  if (error) throw std::runtime_error("cache identity timestamp failed: " + error.message());
  const auto content = path.generic_string() + "|" + std::to_string(size) + "|" + std::to_string(static_cast<long long>(modified));
  const auto run_id = content;
  return {{path, size, modified, content}, {content, "cli-aubio-v1", "default", "offline"}, run_id};
}
std::string encode_cached_analysis(const AnalysisView& view) {
  std::ostringstream out;
  out << "v2 " << view.result.metadata.sample_rate << ' ' << view.result.metadata.channels << ' ' << view.result.metadata.frames << ' '
      << std::quoted(view.result.metadata.format) << ' ' << (view.result.bpm ? *view.result.bpm : -1.0f) << ' '
      << view.result.beats << ' ' << static_cast<int>(view.result.state) << ' ' << view.beat_frames.size();
  for (const auto frame : view.beat_frames) out << ' ' << frame;
  out << ' ' << view.waveform.size();
  for (const auto value : view.waveform) out << ' ' << value;
  out << ' ' << std::quoted(view.key.value_or(std::string{})) << ' ' << std::quoted(view.chord.value_or(std::string{})) << ' ' << view.tonal_history.size();
  for (const auto& label : view.tonal_history) out << ' ' << std::quoted(label);
  return out.str();
}
std::optional<AnalysisView> decode_cached_analysis(const std::string& encoded, const std::filesystem::path& path) {
  std::istringstream in(encoded);
  AnalysisView view;
  std::string version;
  float bpm = -1.0f;
  int state = 0;
  std::size_t beat_count = 0, waveform_count = 0, history_count = 0;
  if (!(in >> version) || version != "v2" || !(in >> view.result.metadata.sample_rate >> view.result.metadata.channels >> view.result.metadata.frames >> std::quoted(view.result.metadata.format) >> bpm >> view.result.beats >> state >> beat_count)) return std::nullopt;
  view.beat_frames.resize(beat_count);
  for (auto& frame : view.beat_frames) if (!(in >> frame)) return std::nullopt;
  if (!(in >> waveform_count)) return std::nullopt;
  view.waveform.resize(waveform_count);
  for (auto& value : view.waveform) if (!(in >> value)) return std::nullopt;
  std::string key, chord;
  if (!(in >> std::quoted(key) >> std::quoted(chord) >> history_count)) return std::nullopt;
  if (!key.empty()) view.key = std::move(key);
  if (!chord.empty()) view.chord = std::move(chord);
  view.tonal_history.resize(history_count);
  for (auto& label : view.tonal_history) if (!(in >> std::quoted(label))) return std::nullopt;
  view.result.bpm = bpm >= 0.0f ? std::optional<float>(bpm) : std::nullopt;
  view.result.state = static_cast<beatglass::TrackingState>(state);
  return view;
}
#endif
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
  constexpr int width = 72;
  // Keep the chart compact enough that Live retains all controls in a normal
  // 36-row terminal snapshot, while making the signal legible without a
  // graphical terminal. Beat markers and the playback cursor use independent
  // rows so they never erase the amplitude shape.
  static constexpr std::array<std::string_view, 9> levels = {" ", "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
  std::vector<int> points(static_cast<std::size_t>(width), 1);
  for (int x = 0; x < width; ++x) {
    const auto sample = waveform.empty() ? 0.08f : waveform[std::min<std::size_t>(waveform.size() - 1, static_cast<std::size_t>(x * waveform.size() / width))];
    // Square-root companding keeps quiet real-world PCM detail visible without changing analysis input.
    points[static_cast<std::size_t>(x)] = std::clamp(static_cast<int>(std::sqrt(std::clamp(sample, 0.0f, 1.0f)) * 8.0f), 1, 8);
  }
  Elements waveform_cells, marker_cells;
  waveform_cells.reserve(width); marker_cells.reserve(width);
  for (int x = 0; x < width; ++x) {
    const auto point = points[static_cast<std::size_t>(x)];
    waveform_cells.push_back(text(std::string(levels[static_cast<std::size_t>(point)])) | color(point > 5 ? Color::GreenLight : Color::Cyan) | bgcolor(Color::RGB(8, 28, 25)));
    bool beat = false;
    if (total_frames) for (const auto frame : beat_frames) if (static_cast<std::uint64_t>(x) * total_frames / width <= frame && frame < static_cast<std::uint64_t>(x + 1) * total_frames / width) { beat = true; break; }
    const auto cursor = playback_fraction >= 0.0f && playback_fraction <= 1.0f && x == std::clamp(static_cast<int>(playback_fraction * width), 0, width - 1);
    marker_cells.push_back(text(cursor ? "◆" : beat ? "│" : " ") | color(cursor ? Color::Cyan : Color::YellowLight) | bgcolor(Color::RGB(8, 28, 25)));
  }
  return vbox({text("WAVEFORM") | bold | underlined | color(Color::GreenLight),
               hbox(std::move(waveform_cells)) | borderRounded,
               hbox(std::move(marker_cells)),
               text("beats: " + std::to_string(beat_frames.size()) + "   playback: " + (playback_fraction >= 0.0f ? "^" : "—")) | color(Color::YellowLight) | italic}) | flex;
}

Element spectrum_chart(const std::vector<float>& spectrum) {
  if (spectrum.empty()) return text("Spectrum waiting for captured audio…") | color(Color::GrayLight) | italic;
  Elements bars;
  for (const auto value : spectrum) {
    const auto height = std::clamp(static_cast<int>(std::round(value * 8.0f)), 0, 8);
    bars.push_back(text(std::string(static_cast<std::size_t>(height), '#')) | color(Color::MagentaLight));
  }
  return hbox(bars) | bgcolor(Color::RGB(20, 14, 28));
}
Element spectrum_history_chart(const std::vector<std::vector<float>>& history) {
  if (history.empty()) return text("FFT history waiting for captured audio…") | color(Color::GrayLight) | italic;
  Elements rows;
  for (const auto& frame : history) {
    std::string row;
    for (const auto value : frame) row += value > 0.75f ? "█" : value > 0.45f ? "▓" : value > 0.2f ? "▒" : value > 0.05f ? "░" : " ";
    rows.push_back(text(std::move(row)) | color(Color::MagentaLight));
  }
  return vbox(std::move(rows)) | bgcolor(Color::RGB(20, 14, 28));
}
std::vector<Track> discover(const std::filesystem::path& root, bool recursive = false) {
  std::vector<Track> tracks;
  if (root.empty() || !std::filesystem::is_directory(root)) return tracks;
  const auto visit = [&](const auto& entry) {
    if (!entry.is_regular_file()) return;
    const auto ext = entry.path().extension().string();
    if (ext == ".wav" || ext == ".flac" || ext == ".mp3" || ext == ".aiff" || ext == ".aif") {
      Track track{entry.path(), entry.path().stem().string(), std::nullopt};
      try { track.metadata = beatglass::read_metadata(entry.path()); } catch (const std::exception&) { /* Keep browseability; analysis reports the full decoder error. */ }
      tracks.push_back(std::move(track));
    }
  };
  if (recursive) for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) visit(entry);
  else for (const auto& entry : std::filesystem::directory_iterator(root)) visit(entry);
  std::ranges::sort(tracks, {}, &Track::title); return tracks;
}
std::vector<Track> discover(const std::vector<std::filesystem::path>& roots, bool recursive = true) {
  std::vector<Track> tracks;
  std::set<std::filesystem::path> seen;
  for (const auto& root : roots) {
    for (auto& track : discover(root, recursive)) {
      const auto normalized = std::filesystem::weakly_canonical(track.path);
      if (seen.insert(normalized).second) tracks.push_back(std::move(track));
    }
  }
  std::ranges::sort(tracks, [](const Track& left, const Track& right) { return left.title == right.title ? left.path.string() < right.path.string() : left.title < right.title; });
  return tracks;
}
std::string lower(std::string value) {
  std::ranges::transform(value, value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}
std::string join_history(const std::vector<std::string>& values) {
  if (values.empty()) return "—";
  std::string joined;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i) joined += "  →  ";
    joined += values[i];
  }
  return joined;
}
struct TonalEstimate { std::string key; std::string chord; };
TonalEstimate estimate_tonal_labels(const std::vector<float>& spectrum, std::uint32_t sample_rate) {
  static constexpr std::array<const char*, 12> names{"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  if (spectrum.size() < 4 || sample_rate == 0) return {};
  std::array<float, 12> pitch{};
  for (std::size_t bin = 1; bin < spectrum.size(); ++bin) {
    // Live keeps a bounded display spectrum by resampling the full FFT.  These
    // bins therefore span 0..Nyquist, rather than retaining the source FFT bin
    // numbers; map them back through the display spectrum's actual frequency
    // range before assigning pitch classes.
    const auto frequency = static_cast<float>(bin) * static_cast<float>(sample_rate) /
        (2.0f * static_cast<float>(spectrum.size() - 1));
    if (frequency < 55.0f || frequency > 1760.0f) continue;
    const auto midi = 69.0f + 12.0f * std::log2(frequency / 440.0f);
    const auto pitch_class = static_cast<int>(std::lround(midi)) % 12;
    pitch[static_cast<std::size_t>((pitch_class + 12) % 12)] += spectrum[bin];
  }
  const auto strongest = std::distance(pitch.begin(), std::max_element(pitch.begin(), pitch.end()));
  if (pitch[static_cast<std::size_t>(strongest)] <= 0.001f) return {};
  static constexpr std::array<float, 12> major{6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f};
  static constexpr std::array<float, 12> minor{6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f};
  auto score = [&](const auto& profile, int root) { float value = 0.0f; for (int i = 0; i < 12; ++i) value += pitch[static_cast<std::size_t>((i + root) % 12)] * profile[static_cast<std::size_t>(i)]; return value; };
  int major_root = 0, minor_root = 0; float major_score = -1.0f, minor_score = -1.0f;
  for (int root = 0; root < 12; ++root) { if (const auto value = score(major, root); value > major_score) major_score = value, major_root = root; if (const auto value = score(minor, root); value > minor_score) minor_score = value, minor_root = root; }
  const auto minor_mode = minor_score > major_score;
  const auto root = minor_mode ? minor_root : major_root;
  std::array<std::pair<float, int>, 12> ranked{};
  for (int i = 0; i < 12; ++i) ranked[static_cast<std::size_t>(i)] = {pitch[static_cast<std::size_t>(i)], i};
  std::ranges::sort(ranked, std::greater<>{});
  const auto third = ranked[1].second;
  const auto fifth = ranked[2].second;
  return {std::string(names[root]) + (minor_mode ? " minor" : " major"), std::string(names[root]) + "–" + names[third] + "–" + names[fifth] + " (provisional FFT)"};
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
  void resume() { request(Command::resume); }
  void pause() { request(Command::pause); }
  void seek_seconds(std::int64_t seconds) { { std::lock_guard lock(mutex_); seek_delta_seconds_ = seconds; command_ = Command::seek; } condition_.notify_one(); }
  void stop() { request(Command::stop); }
  void set_device(UINT device_id) { std::lock_guard lock(mutex_); device_id_ = device_id; }
  beatglass::PlaybackState state() const noexcept { return state_.load(std::memory_order_acquire); }
  std::uint64_t epoch() const noexcept { return epoch_.load(std::memory_order_acquire); }
  std::uint64_t current_frame() const noexcept { return current_frame_.load(std::memory_order_acquire); }
  std::uint64_t total_frames() const noexcept { return total_frames_.load(std::memory_order_acquire); }
  std::string status() const { std::lock_guard lock(mutex_); return status_; }
private:
  enum class Command { none, play, resume, pause, seek, stop, shutdown };
  void request(Command command) { { std::lock_guard lock(mutex_); command_ = command; } condition_.notify_one(); }
  Command take_command() { std::lock_guard lock(mutex_); const auto command = command_; command_ = Command::none; return command; }
  std::int64_t take_seek_delta_seconds() { std::lock_guard lock(mutex_); const auto delta = seek_delta_seconds_; seek_delta_seconds_ = 0; return delta; }
  UINT take_device_id() { std::lock_guard lock(mutex_); return device_id_; }
  void set_status(std::string status) { std::lock_guard lock(mutex_); status_ = std::move(status); }
  void run() {
    std::unique_ptr<beatglass::FileReader> reader;
    beatglass::windows::WindowsPlaybackSession output;
    // Keep the reader and renderer inside WindowsPlaybackSession's shared
    // bounded queue contract; passing 8192 here makes start_device reject the
    // session before the native output can open.
    std::vector<float> decoded(4096), render(4096);
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
          if (!output.start_device(take_device_id(), metadata.sample_rate, static_cast<std::uint16_t>(metadata.channels), decoded.size(), next_epoch, error)) { set_status("Playback unavailable: " + error); reader.reset(); continue; }
          if (!session_.start(next_epoch)) { output.stop(); reader.reset(); set_status("Playback session could not acquire a fresh epoch."); continue; }
          epoch_.store(next_epoch, std::memory_order_release); state_.store(beatglass::PlaybackState::playing, std::memory_order_release); set_status("Playing decoded PCM through native output.");
        } catch (const std::exception& error) { set_status(std::string("Playback failed: ") + error.what()); reader.reset(); continue; }
      } else if (command == Command::resume) {
        if (output.resume() && session_.resume()) { state_.store(beatglass::PlaybackState::playing, std::memory_order_release); set_status("Playback resumed."); }
      } else if (command == Command::pause) {
        if (output.pause() && session_.pause()) { state_.store(beatglass::PlaybackState::paused, std::memory_order_release); set_status("Playback paused."); }
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
          if (!output.start_device(take_device_id(), metadata.sample_rate, static_cast<std::uint16_t>(metadata.channels), decoded.size(), next_epoch, error)) throw std::runtime_error(error);
          if (!session_.seek(target, next_epoch)) throw std::runtime_error("Playback session rejected the seek epoch");
          current_frame = target; epoch_.store(next_epoch, std::memory_order_release); state_.store(beatglass::PlaybackState::playing, std::memory_order_release); set_status("Playback seeked to frame " + std::to_string(target) + ".");
          current_frame_.store(target, std::memory_order_release);
        } catch (const std::exception& error) { set_status(std::string("Playback seek failed: ") + error.what()); }
      } else if (command == Command::stop) {
        output.stop(); session_.stop(); reader.reset(); current_frame_.store(0, std::memory_order_release); total_frames_.store(0, std::memory_order_release); state_.store(beatglass::PlaybackState::stopped, std::memory_order_release); set_status("Playback stopped.");
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
        if (command == Command::stop || command == Command::shutdown) { output.stop(); session_.stop(); reader.reset(); state_.store(beatglass::PlaybackState::stopped, std::memory_order_release); shutdown = command == Command::shutdown; break; }
        if (command == Command::pause) { output.pause(); session_.pause(); state_.store(beatglass::PlaybackState::paused, std::memory_order_release); set_status("Playback paused."); break; }
        if (command == Command::play || command == Command::seek) {
          output.stop();
          reader.reset();
          session_.stop();
          state_.store(beatglass::PlaybackState::stopped, std::memory_order_release);
          // Preserve the command for the outer worker loop after the current
          // queue-drain boundary; a new source/seek must never be discarded.
          { std::lock_guard lock(mutex_); command_ = command; }
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
      }
      if (reader && state_.load(std::memory_order_acquire) == beatglass::PlaybackState::playing) {
        const auto frame_count = read.block.samples.size() / std::max<std::size_t>(1, read.block.format.channels);
        if (!session_.accept(read.block.epoch, read.block.first_frame, frame_count)) {
          output.stop(); session_.stop(); reader.reset(); state_.store(beatglass::PlaybackState::stopped, std::memory_order_release);
          set_status("Playback stopped after an out-of-order decoded block.");
          continue;
        }
        current_frame = session_.next_frame();
        current_frame_.store(current_frame, std::memory_order_release);
      }
      if (shutdown || !reader || state_.load(std::memory_order_acquire) != beatglass::PlaybackState::playing) continue;
      output.render(render);
    }
    output.stop();
    session_.stop();
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
  UINT device_id_{WAVE_MAPPER};
  beatglass::PlaybackSession session_;
};
#endif
#ifdef __APPLE__
class CliPlayback final {
public:
  CliPlayback() : worker_([this] { run(); }) {}
  ~CliPlayback() {
    { std::lock_guard lock(mutex_); command_ = Command::shutdown; }
    condition_.notify_one();
    worker_.join();
  }
  void play(const std::filesystem::path& path) { { std::lock_guard lock(mutex_); path_ = path; command_ = Command::play; } condition_.notify_one(); }
  void resume() { request(Command::resume); }
  void pause() { request(Command::pause); }
  void seek_seconds(std::int64_t seconds) { { std::lock_guard lock(mutex_); seek_delta_seconds_ = seconds; command_ = Command::seek; } condition_.notify_one(); }
  void stop() { request(Command::stop); }
  beatglass::PlaybackState state() const noexcept { return state_.load(std::memory_order_acquire); }
  std::uint64_t epoch() const noexcept { return epoch_.load(std::memory_order_acquire); }
  std::uint64_t current_frame() const noexcept { return current_frame_.load(std::memory_order_acquire); }
  std::uint64_t total_frames() const noexcept { return total_frames_.load(std::memory_order_acquire); }
  std::string status() const { std::lock_guard lock(mutex_); return status_; }
private:
  enum class Command { none, play, resume, pause, seek, stop, shutdown };
  void request(Command command) { { std::lock_guard lock(mutex_); command_ = command; } condition_.notify_one(); }
  Command take_command() { std::lock_guard lock(mutex_); const auto command = command_; command_ = Command::none; return command; }
  std::int64_t take_seek_delta_seconds() { std::lock_guard lock(mutex_); const auto delta = seek_delta_seconds_; seek_delta_seconds_ = 0; return delta; }
  void set_status(std::string status) { std::lock_guard lock(mutex_); status_ = std::move(status); }
  void run() {
    std::unique_ptr<beatglass::FileReader> reader;
    beatglass::macos::MacAudioUnitPlayback output;
    std::vector<float> decoded(4096);
    std::uint64_t next_epoch = 0;
    bool shutdown = false;
    while (!shutdown) {
      const auto command = take_command();
      if (command == Command::shutdown) break;
      if (command == Command::play) {
        output.stop(); session_.stop(); reader.reset(); state_.store(beatglass::PlaybackState::stopped, std::memory_order_release);
        try {
          reader = std::make_unique<beatglass::FileReader>(path_, ++next_epoch);
          const auto& metadata = reader->metadata();
          current_frame_.store(0, std::memory_order_release);
          total_frames_.store(metadata.frames, std::memory_order_release);
          if (!output.start(metadata.sample_rate, static_cast<std::uint16_t>(metadata.channels), decoded.size(), next_epoch)) { set_status("Playback unavailable: macOS AudioUnit could not start."); reader.reset(); continue; }
          if (!session_.start(next_epoch)) { output.stop(); reader.reset(); set_status("Playback session could not acquire a fresh epoch."); continue; }
          epoch_.store(next_epoch, std::memory_order_release); state_.store(beatglass::PlaybackState::playing, std::memory_order_release); set_status("Playing decoded PCM through macOS AudioUnit output.");
        } catch (const std::exception& error) { set_status(std::string("Playback failed: ") + error.what()); reader.reset(); continue; }
      } else if (command == Command::resume) {
        if (reader && output.resume() && session_.resume()) { state_.store(beatglass::PlaybackState::playing, std::memory_order_release); set_status("Playback resumed."); }
      } else if (command == Command::pause) {
        if (reader && output.pause() && session_.pause()) { state_.store(beatglass::PlaybackState::paused, std::memory_order_release); set_status("Playback paused."); }
      } else if (command == Command::seek && reader) {
        try {
          const auto metadata = reader->metadata();
          const auto delta = take_seek_delta_seconds() * static_cast<std::int64_t>(metadata.sample_rate);
          const auto current = current_frame_.load(std::memory_order_acquire);
          const auto target = delta < 0 && current < static_cast<std::uint64_t>(-delta) ? 0 : delta < 0 ? current - static_cast<std::uint64_t>(-delta) : std::min<std::uint64_t>(metadata.frames, current + static_cast<std::uint64_t>(delta));
          output.stop(); reader->seek(target, ++next_epoch);
          if (!output.start(metadata.sample_rate, static_cast<std::uint16_t>(metadata.channels), decoded.size(), next_epoch)) throw std::runtime_error("macOS AudioUnit could not restart");
          if (!session_.seek(target, next_epoch)) throw std::runtime_error("Playback session rejected the seek epoch");
          current_frame_.store(target, std::memory_order_release); epoch_.store(next_epoch, std::memory_order_release); state_.store(beatglass::PlaybackState::playing, std::memory_order_release); set_status("Playback seeked to frame " + std::to_string(target) + ".");
        } catch (const std::exception& error) { set_status(std::string("Playback seek failed: ") + error.what()); }
      } else if (command == Command::stop) {
        output.stop(); session_.stop(); reader.reset(); current_frame_.store(0, std::memory_order_release); total_frames_.store(0, std::memory_order_release); state_.store(beatglass::PlaybackState::stopped, std::memory_order_release); set_status("Playback stopped.");
      }
      if (!reader || state_.load(std::memory_order_acquire) != beatglass::PlaybackState::playing) { std::unique_lock lock(mutex_); condition_.wait_for(lock, std::chrono::milliseconds(20), [&] { return command_ != Command::none; }); continue; }
      const auto read = reader->read(decoded);
      if (read.status == beatglass::FileReadStatus::end) { output.stop(); session_.stop(); reader.reset(); state_.store(beatglass::PlaybackState::stopped, std::memory_order_release); set_status("Playback reached end of file."); continue; }
      if (read.status != beatglass::FileReadStatus::data) continue;
      while (!output.submit(read.block)) {
        const auto pending = take_command();
        if (pending == Command::stop || pending == Command::shutdown) { output.stop(); session_.stop(); reader.reset(); state_.store(beatglass::PlaybackState::stopped, std::memory_order_release); shutdown = pending == Command::shutdown; break; }
        if (pending == Command::pause) { output.pause(); session_.pause(); state_.store(beatglass::PlaybackState::paused, std::memory_order_release); set_status("Playback paused."); break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
      }
      if (reader && state_.load(std::memory_order_acquire) == beatglass::PlaybackState::playing) {
        const auto frame_count = read.block.samples.size() / std::max<std::size_t>(1, read.block.format.channels);
        if (!session_.accept(read.block.epoch, read.block.first_frame, frame_count)) {
          output.stop(); session_.stop(); reader.reset(); state_.store(beatglass::PlaybackState::stopped, std::memory_order_release);
          set_status("Playback stopped after an out-of-order decoded block.");
          continue;
        }
        current_frame_.store(session_.next_frame(), std::memory_order_release);
      }
    }
    output.stop();
    session_.stop();
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
  beatglass::PlaybackSession session_;
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
  const auto preferences = load_preferences();
  const bool explicit_root = snapshot ? argc > 2 : argc > 1;
  std::filesystem::path root = snapshot ? (argc > 2 ? argv[2] : std::filesystem::path{}) : (argc > 1 ? argv[1] : std::filesystem::path{});
  std::vector<std::filesystem::path> library_roots;
  if (!explicit_root) {
    for (const auto& folder : preferences.library_folders) library_roots.emplace_back(folder);
    if (library_roots.empty() && !preferences.library_folder.empty()) library_roots.emplace_back(preferences.library_folder);
    if (!library_roots.empty()) root = library_roots.front();
  }
  if (root.empty()) root = std::filesystem::current_path();
  if (library_roots.empty()) library_roots.push_back(root);
  auto tracks = discover(library_roots, true);
  bool sort_by_path = false;
  std::string folder_text = root.string();
  std::string filter;
  bool recursive = true;
  std::vector<std::string> live_endpoints;
  std::vector<std::string> live_endpoint_ids;
  std::string live_error;
  std::string live_endpoint_preference = preferences.live_endpoint_id;
  std::string live_endpoint_name_preference = preferences.live_endpoint_name;
  float live_bpm_multiplier = preferences.bpm_multiplier < 0 ? 0.5f : preferences.bpm_multiplier > 0 ? 2.0f : 1.0f;
#ifdef BEATGLASS_CLI_LIBRARY
  std::unique_ptr<beatglass::library::StorageOwner> library_storage = std::make_unique<beatglass::library::StorageOwner>();
  std::string library_storage_status;
  std::size_t library_retryable_count = 0;
  if (const auto config = preferences_path(); !config.empty()) {
    std::error_code storage_error;
    std::filesystem::create_directories(config.parent_path(), storage_error);
    const auto database = config.parent_path() / "library.sqlite3";
    if (!library_storage->open({database}, &library_storage_status)) library_storage.reset();
    else library_retryable_count = library_storage->retryable_runs().size();
  }
  if (library_storage) {
#if defined(BEATGLASS_LIBRARY_FINGERPRINT)
    for (const auto& track : tracks) {
      std::string catalog_diagnostic;
      library_storage->refresh_collection_file(track.path, &catalog_diagnostic);
    }
#endif
  }
#endif
#ifdef _WIN32
  auto live_endpoint_snapshot = beatglass::windows::enumerate_render_endpoints();
  live_error = live_endpoint_snapshot.error;
  for (const auto& endpoint : live_endpoint_snapshot.endpoints) {
    live_endpoints.push_back(endpoint.name + (endpoint.is_default ? " (default)" : ""));
    live_endpoint_ids.push_back(endpoint.id);
  }
#else
#if defined(__APPLE__)
  auto live_endpoint_snapshot = beatglass::coreaudio::make_native_backend()->enumerate();
  live_error = live_endpoint_snapshot.diagnostic;
  for (const auto& endpoint : live_endpoint_snapshot.sources) { live_endpoints.push_back(endpoint.name); live_endpoint_ids.push_back(endpoint.id); }
#elif defined(BEATGLASS_HAS_PIPEWIRE)
  auto live_endpoint_snapshot = beatglass::pipewire::discover_sinks(std::chrono::milliseconds(500));
  live_error = live_endpoint_snapshot.error;
  for (const auto& endpoint : live_endpoint_snapshot.sinks) {
    live_endpoints.push_back(endpoint.name.empty() ? endpoint.description : endpoint.name);
    live_endpoint_ids.push_back(endpoint.serial);
  }
  if (live_endpoints.empty() && live_error.empty()) live_error = "PipeWire found no output sink monitors.";
#else
  live_endpoints.push_back("PipeWire monitor (build with BEATGLASS_BUILD_PIPEWIRE=ON)");
  live_error = "Native sink enumeration is unavailable in this build.";
#endif
#endif
  int tab = 0, selected = tracks.empty() ? -1 : 0;
  int live_selected = 0;
  bool live_endpoint_identity_found = false;
#ifdef _WIN32
  for (int index = 0; index < static_cast<int>(live_endpoint_snapshot.endpoints.size()); ++index) {
    if (live_endpoint_snapshot.endpoints[index].id == live_endpoint_preference) { live_selected = index; live_endpoint_identity_found = true; }
    else if (!live_endpoint_identity_found && (live_endpoint_snapshot.endpoints[index].name == live_endpoint_name_preference ||
             (live_endpoint_preference.empty() && live_endpoint_name_preference.empty() && live_endpoint_snapshot.endpoints[index].is_default))) live_selected = index;
  }
#elif defined(__APPLE__)
  for (int index = 0; index < static_cast<int>(live_endpoint_snapshot.sources.size()); ++index) {
    if (live_endpoint_snapshot.sources[index].id == live_endpoint_preference) { live_selected = index; live_endpoint_identity_found = true; }
    else if (!live_endpoint_identity_found && live_endpoint_snapshot.sources[index].name == live_endpoint_name_preference) live_selected = index;
  }
#elif defined(BEATGLASS_HAS_PIPEWIRE)
  for (int index = 0; index < static_cast<int>(live_endpoint_snapshot.sinks.size()); ++index)
    if (live_endpoint_snapshot.sinks[static_cast<std::size_t>(index)].serial == live_endpoint_preference) { live_selected = index; live_endpoint_identity_found = true; }
    else if (!live_endpoint_identity_found && (live_endpoint_snapshot.sinks[static_cast<std::size_t>(index)].name == live_endpoint_name_preference || live_endpoint_snapshot.sinks[static_cast<std::size_t>(index)].description == live_endpoint_name_preference)) live_selected = index;
#endif
  std::string status = tracks.empty() ? "Choose a folder with audio files to begin." : "Ready to analyze a track.";
  std::size_t library_cache_hits = 0;
  std::optional<beatglass::TempoResult> result;
  std::vector<float> waveform;
  std::vector<std::uint64_t> beat_frames;
  std::optional<std::string> key;
  std::optional<std::string> chord;
  std::vector<std::string> tonal_history;
  std::filesystem::path analysis_path;
  bool busy = false;
  bool reduce_motion = preferences.reduce_motion;
#if defined(_WIN32) || defined(__APPLE__) || defined(BEATGLASS_HAS_PIPEWIRE)
  CliPlayback playback;
#ifdef _WIN32
  const auto playback_devices = beatglass::windows::enumerate_waveout_devices();
  std::vector<std::string> playback_device_labels;
  for (const auto& device : playback_devices) playback_device_labels.push_back(std::to_string(device.id) + ": " + device.name);
  int playback_device_selected = 0;
  for (int index = 0; index < static_cast<int>(playback_devices.size()); ++index) if (playback_devices[static_cast<std::size_t>(index)].id == preferences.playback_device_id) playback_device_selected = index;
#else
  std::vector<std::string> playback_device_labels{"Default macOS output (AudioUnit)"};
  int playback_device_selected = 0;
#endif
#else
  beatglass::PlaybackSession playback;
  std::vector<std::string> playback_device_labels;
  int playback_device_selected = 0;
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
  std::map<std::filesystem::path, std::string> analysis_errors;
  std::vector<std::string> library_labels;
  std::vector<int> library_indices;
  int library_visible_selected = 0;
#ifdef _WIN32
  beatglass::SourceEpochAllocator live_epochs;
  std::unique_ptr<beatglass::windows::LoopbackSession> live_session;
  std::unique_ptr<beatglass::CausalTempoReplay> live_replay = std::make_unique<beatglass::CausalTempoReplay>();
#ifdef BEATGLASS_CLI_MUSIC_REPLAY
  std::unique_ptr<beatglass::CausalMusicReplay> live_music_replay = std::make_unique<beatglass::CausalMusicReplay>();
#endif
  std::optional<std::string> live_key;
  std::optional<std::string> live_chord;
  std::vector<std::string> live_tonal_history;
  std::string live_status = "Capture stopped.";
  std::optional<float> live_bpm;
  beatglass::PresentationState live_presentation;
  beatglass::TrackingState live_tracking_state = beatglass::TrackingState::stopped;
  std::uint32_t live_sample_rate = 0;
  std::uint64_t live_observed_frames = 0;
  std::uint64_t live_discontinuities = 0;
  std::uint64_t live_beats = 0;
  float live_signal_confidence = 0.0f;
  std::vector<float> live_waveform(160, 0.0f);
  std::vector<float> live_spectrum(32, 0.0f);
  std::vector<std::vector<float>> live_spectrum_history;
  std::vector<float> live_tempo_history;
  std::chrono::steady_clock::time_point live_last_packet{};
  std::chrono::steady_clock::time_point live_next_reacquire{};
  bool live_has_packet = false;
  std::optional<beatglass::VisualizationFrame> live_visualization_frame;
  beatglass::VisualizationPresentation live_visualization_presentation;
  std::uint64_t live_visualization_revision = 0;
  beatglass::VisualizationProcessor live_visualization([](void*) noexcept {}, nullptr);
  live_session = std::make_unique<beatglass::windows::LoopbackSession>(live_epochs, [&] {
    live_replay = std::make_unique<beatglass::CausalTempoReplay>();
    live_bpm.reset();
    live_tracking_state = beatglass::TrackingState::reacquiring;
    live_sample_rate = 0; live_observed_frames = 0; live_discontinuities = 0;
    live_beats = 0; live_tempo_history.clear(); live_spectrum.assign(live_spectrum.size(), 0.0f);
    live_spectrum_history.clear();
    live_visualization.reset(); live_visualization_frame.reset();
    live_visualization_presentation.clear();
    live_visualization_revision = 0;
    live_signal_confidence = 0.0f;
  });
#else
#if defined(BEATGLASS_HAS_PIPEWIRE)
  std::unique_ptr<beatglass::pipewire::MonitorBackend> live_pipewire = beatglass::pipewire::make_monitor_backend();
  std::uint64_t live_pipewire_epoch = 1;
#elif defined(__APPLE__)
  std::unique_ptr<MacMonitorBackend> live_pipewire = std::make_unique<MacMonitorBackend>();
  std::uint64_t live_pipewire_epoch = 1;
#endif
  std::unique_ptr<beatglass::CausalTempoReplay> live_replay = std::make_unique<beatglass::CausalTempoReplay>();
#ifdef BEATGLASS_CLI_MUSIC_REPLAY
  std::unique_ptr<beatglass::CausalMusicReplay> live_music_replay = std::make_unique<beatglass::CausalMusicReplay>();
#endif
  std::optional<std::string> live_key;
  std::optional<std::string> live_chord;
  std::vector<std::string> live_tonal_history;
  std::string live_status = "Native loopback capture is unavailable in this build.";
  std::optional<float> live_bpm;
  beatglass::PresentationState live_presentation;
  beatglass::TrackingState live_tracking_state = beatglass::TrackingState::stopped;
  std::uint32_t live_sample_rate = 0;
  std::uint64_t live_observed_frames = 0;
  std::uint64_t live_discontinuities = 0;
  std::uint64_t live_beats = 0;
  float live_signal_confidence = 0.0f;
  std::vector<float> live_waveform(160, 0.0f);
  std::vector<float> live_spectrum(32, 0.0f);
  std::vector<std::vector<float>> live_spectrum_history;
  std::vector<float> live_tempo_history;
  std::chrono::steady_clock::time_point live_last_packet{};
  std::chrono::steady_clock::time_point live_next_reacquire{};
  bool live_has_packet = false;
  std::optional<beatglass::VisualizationFrame> live_visualization_frame;
  beatglass::VisualizationPresentation live_visualization_presentation;
  std::uint64_t live_visualization_revision = 0;
  beatglass::VisualizationProcessor live_visualization([](void*) noexcept {}, nullptr);
#endif
  std::string live_tonal_candidate_key;
  std::string live_tonal_candidate_chord;
  unsigned live_tonal_candidate_streak = 0;
  auto update_fft_tonal = [&] {
    const auto estimate = estimate_tonal_labels(live_spectrum, live_sample_rate);
    if (estimate.key.empty() || estimate.chord.empty()) return;
    if (estimate.key == live_tonal_candidate_key && estimate.chord == live_tonal_candidate_chord) {
      live_tonal_candidate_streak = std::min(3u, live_tonal_candidate_streak + 1u);
    } else {
      live_tonal_candidate_key = estimate.key;
      live_tonal_candidate_chord = estimate.chord;
      live_tonal_candidate_streak = 1;
    }
    // Require three consecutive display frames before changing a displayed
    // label. This keeps the causal FFT fallback responsive to real changes
    // while avoiding single-frame spectral spikes.
    if (live_tonal_candidate_streak < 3) return;
    const auto changed = !live_key || !live_chord || *live_key != estimate.key || *live_chord != estimate.chord;
    if (!changed) return;
    live_key = estimate.key;
    live_chord = estimate.chord;
    live_tonal_history.push_back(estimate.chord);
    if (live_tonal_history.size() > 8) live_tonal_history.erase(live_tonal_history.begin());
  };
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
#ifdef BEATGLASS_CLI_LIBRARY
    auto* library_storage_ptr = library_storage.get();
#endif
    analysis_future = std::async(std::launch::async, [paths = std::move(paths), stop_token, &analysis_frames, &analysis_total, &batch_completed
#ifdef BEATGLASS_CLI_LIBRARY
      , library_storage_ptr
#endif
      ] {
      AnalysisBatch batch;
      for (const auto& path : paths) {
        if (stop_token.stop_requested()) break;
#ifdef BEATGLASS_CLI_LIBRARY
        std::optional<CacheSource> cache_identity;
#endif
        try {
        const auto metadata = beatglass::read_metadata(path);
        analysis_total.store(metadata.frames, std::memory_order_release);
#ifdef BEATGLASS_CLI_LIBRARY
        if (library_storage_ptr) {
          cache_identity = cache_source(path);
          const auto admission = library_storage_ptr->admit_analysis(cache_identity->run_id, cache_identity->identity, path.string(), beatglass::library::kCliDecoderSnapshot);
          if (admission.cached) {
            if (auto cached = decode_cached_analysis(*admission.cached, path)) {
              if (cached->waveform.empty()) cached->waveform = decode_waveform(path, stop_token);
              cached->from_persistent_cache = true;
              batch.results.emplace_back(path, std::move(*cached));
              ++batch.cache_hits;
              batch_completed.fetch_add(1, std::memory_order_acq_rel);
              continue;
            }
          }
        }
#endif
        std::vector<std::uint64_t> beat_frames;
        std::optional<std::string> key;
        std::optional<std::string> chord;
        std::vector<std::string> tonal_history;
#ifdef BEATGLASS_CLI_MUSIC_REPLAY
        const auto tonal = beatglass::replay_music_file(path, 1, [&](const beatglass::CausalMusicBeat& beat) { beat_frames.push_back(beat.frame); }, stop_token,
          [&](const beatglass::CausalTonalFrame& frame) { key = frame.rolling_key; chord = frame.local_chord; if (frame.local_chord) { tonal_history.push_back(*frame.local_chord); if (tonal_history.size() > 8) tonal_history.erase(tonal_history.begin()); } },
          [&](const beatglass::CausalMusicState& state) { analysis_frames.store(state.tempo.observed_through, std::memory_order_release); });
        const beatglass::TempoResult result{metadata, tonal.tempo.bpm, static_cast<unsigned>(std::min<std::uint64_t>(tonal.tempo.beats, std::numeric_limits<unsigned>::max())), tonal.tempo.state};
#else
        const auto result = beatglass::replay_file(path, [&](std::uint64_t observed, const beatglass::TempoResult&) { analysis_frames.store(observed, std::memory_order_release); }, [&](const beatglass::ReplayBeat& beat) { beat_frames.push_back(beat.frame); }, stop_token);
#endif
        auto waveform = decode_waveform(path, stop_token);
        if (stop_token.stop_requested()) break;
        AnalysisView completed{result, std::move(waveform), std::move(beat_frames), std::move(key), std::move(chord), std::move(tonal_history)};
#ifdef BEATGLASS_CLI_LIBRARY
        if (library_storage_ptr && cache_identity) {
          const auto identity = cache_identity->identity;
          beatglass::library::AnalysisOperation operation;
          operation.source = cache_identity->source;
          operation.identity = identity;
          operation.decoder_snapshot = beatglass::library::kCliDecoderSnapshot;
          const auto encoded = encode_cached_analysis(completed);
          operation.serialize = [encoded] { return encoded; };
          operation.verify = [source = cache_identity->source](std::stop_token cancel) {
            if (cancel.stop_requested()) return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::cancelled, "cancelled"};
            std::error_code error;
            const auto size = std::filesystem::file_size(source.path, error);
            const auto modified = std::filesystem::last_write_time(source.path, error).time_since_epoch().count();
            if (error) return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::error, error.message()};
            if (size != source.size_bytes || modified != source.modified_ns) return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::changed, "source changed during analysis"};
            return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match, {}};
          };
          const auto published = library_storage_ptr->publish_analysis(cache_identity->run_id, operation, stop_token);
          if (published == beatglass::library_detail::Publish::stale_input) throw std::runtime_error("source changed during analysis; result was not cached");
        }
#endif
        batch.results.emplace_back(path, std::move(completed));
        batch_completed.fetch_add(1, std::memory_order_acq_rel);
        } catch (const std::exception& error) {
          if (stop_token.stop_requested()) break;
#ifdef BEATGLASS_CLI_LIBRARY
          if (library_storage_ptr && cache_identity && library_storage_ptr->cache())
            library_storage_ptr->cache()->mark_retryable(cache_identity->run_id, {cache_identity->identity.content_hash, cache_identity->identity.engine_version, cache_identity->identity.configuration_id, cache_identity->identity.mode});
#endif
          batch.errors.emplace_back(path, error.what());
          batch_completed.fetch_add(1, std::memory_order_acq_rel);
        }
      }
      return batch;
    });
  };
  auto analyze = [&] {
    if (selected < 0 || selected >= static_cast<int>(tracks.size())) { status = "Select a supported audio file first."; return; }
    analyze_paths({tracks[selected].path});
  };
  ButtonOption compact_button_option;
  compact_button_option.transform = [](const EntryState& state) {
    auto label = text(state.label) | color(state.focused ? Color::White : Color::GrayLight);
    return state.focused ? label | bgcolor(Color::RGB(25, 92, 112)) | bold : label;
  };
  auto analyze_selected_button = Button("Analyze selected", analyze, compact_button_option);
  auto retry_analysis_button = Button("Retry failed", [&] {
    if (selected < 0 || selected >= static_cast<int>(tracks.size()) || !analysis_errors.contains(tracks[selected].path)) {
      status = "Select a failed track before retrying.";
      return;
    }
    analysis_errors.erase(tracks[selected].path);
    status = "Retrying failed analysis…";
    analyze();
  }, compact_button_option);
#ifdef BEATGLASS_CLI_LIBRARY
  auto recover_persisted_button = Button("Recover persisted", [&] {
    if (!library_storage) { status = "Persistent recovery is unavailable; SQLite storage is not open."; return; }
    const auto runs = library_storage->retryable_runs(1);
    if (runs.empty()) { status = "No persisted retryable analysis is waiting for recovery."; return; }
    const auto path = std::filesystem::path(runs.front().source_path);
    const auto found = std::ranges::find(tracks, path, &Track::path);
    if (found == tracks.end()) { status = "Persisted recovery source is no longer in the current Library."; return; }
    selected = static_cast<int>(std::distance(tracks.begin(), found));
    analysis_errors.erase(path);
    status = "Recovering persisted analysis for " + found->title + "…";
    analyze_paths({path});
  }, compact_button_option);
#endif
  auto refresh = [&] {
    const auto cancelling = busy;
    const auto previous_path = selected >= 0 && selected < static_cast<int>(tracks.size())
        ? tracks[static_cast<std::size_t>(selected)].path : std::filesystem::path{};
    if (cancelling) {
      analysis_cancelled.store(true, std::memory_order_release);
      analysis_stop.request_stop();
    }
    tracks = discover(library_roots, recursive);
    if (sort_by_path) std::ranges::sort(tracks, [](const Track& left, const Track& right) { return left.path.generic_string() < right.path.generic_string(); });
    selected = -1;
    if (!previous_path.empty()) {
      for (int index = 0; index < static_cast<int>(tracks.size()); ++index) {
        if (tracks[static_cast<std::size_t>(index)].path == previous_path) { selected = index; break; }
      }
    }
    if (selected < 0 && !tracks.empty()) selected = 0;
    result.reset(); waveform.clear(); beat_frames.clear(); key.reset(); chord.reset(); tonal_history.clear();
    completed_analyses.clear();
    analysis_errors.clear();
    status = std::to_string(tracks.size()) + " tracks indexed; selection preserved by path." +
        (cancelling ? " Cancelling the previous analysis." : "");
  };
  InputOption search_input_option;
  search_input_option.placeholder = "Search library";
  search_input_option.multiline = false;
  search_input_option.transform = [](InputState state) {
    return state.element | color(state.focused ? Color::White : Color::GrayLight) | bgcolor(Color::RGB(18, 24, 31));
  };
  auto filter_input = Input(&filter, search_input_option);
  auto folder_input = Input(&folder_text, "Audio folder");
  auto clear_search_button = Button("Clear search", [&] { filter.clear(); status = "Library search cleared."; }, compact_button_option);
  auto search_bar = Renderer(Container::Horizontal(Components{filter_input, clear_search_button}), [&] {
    return hbox({text("Search: ") | bold | color(Color::Cyan), filter_input->Render() | size(WIDTH, GREATER_THAN, 20), text(" "), action(clear_search_button->Render(), Color::Cyan)});
  });
  std::string seek_step_text = std::to_string(preferences.seek_step);
  std::string visual_mode = preferences.visual_mode;
  auto seek_step_input = Input(&seek_step_text, "seconds");
  auto seek_step = [&] {
    try { return std::clamp(std::stoi(seek_step_text), 1, 60); }
    catch (...) { return 5; }
  };
  auto persist_preferences = [&] {
    const auto selected_device_id = [&] {
#ifdef _WIN32
      return !playback_devices.empty() && playback_device_selected >= 0 && playback_device_selected < static_cast<int>(playback_devices.size()) ? static_cast<int>(playback_devices[static_cast<std::size_t>(playback_device_selected)].id) : -1;
#else
      return -1;
#endif
    }();
    const auto bpm_setting = live_bpm_multiplier < 1.0f ? -1 : live_bpm_multiplier > 1.0f ? 1 : 0;
    // Preserve the last known endpoint identity when discovery is temporarily
    // unavailable.  Rebuilding preferences with empty endpoint fields here
    // used to silently forget the user's selected sink on exit.
    CliPreferences updated{reduce_motion, 5, visual_mode, selected_device_id, bpm_setting,
                           live_endpoint_preference, live_endpoint_name_preference, root.string(), {}};
    for (const auto& folder : library_roots) updated.library_folders.push_back(folder.string());
    try { updated.seek_step = std::clamp(std::stoi(seek_step_text), 1, 60); } catch (...) {}
    if (live_selected >= 0 && live_selected < static_cast<int>(live_endpoint_ids.size())) {
      updated.live_endpoint_id = live_endpoint_ids[static_cast<std::size_t>(live_selected)];
      live_endpoint_preference = updated.live_endpoint_id;
      updated.live_endpoint_name = live_endpoints[static_cast<std::size_t>(live_selected)];
      live_endpoint_name_preference = updated.live_endpoint_name;
    }
    save_preferences(updated);
  };
  bool last_saved_reduce_motion = reduce_motion;
  std::string last_saved_seek_step = seek_step_text;
  std::string last_saved_visual_mode = visual_mode;
  int last_saved_playback_device = playback_device_selected;
  auto recursive_checkbox = Checkbox("Recursive", &recursive);
  auto refresh_button = Button("Refresh", refresh, compact_button_option);
  std::string sort_button_label = " Sort: title ";
  auto sort_button = Button(&sort_button_label, [&] {
    sort_by_path = !sort_by_path;
    sort_button_label = sort_by_path ? " Sort: path " : " Sort: title ";
    refresh();
    status = std::string("Sorted visible Library by ") + (sort_by_path ? "path." : "title.");
  }, compact_button_option);
  auto cancel_analysis_button = Button("Cancel analysis", [&] {
    if (!busy) return;
    analysis_cancelled.store(true, std::memory_order_release);
    analysis_stop.request_stop();
    status = "Cancelling analysis…";
  }, compact_button_option);
  auto analyze_all_button = Button("Analyze all", [&] {
    std::vector<std::filesystem::path> paths;
    for (const auto index : library_indices) paths.push_back(tracks[static_cast<std::size_t>(index)].path);
    analyze_paths(std::move(paths));
  }, compact_button_option);
  auto open_folder_button = Button("Open folder", [&] {
    const std::filesystem::path requested(folder_text);
    std::string error;
    if (!open_folder_in_file_manager(requested, error)) { status = std::move(error); return; }
    root = requested; recursive = true; refresh(); persist_preferences(); status = "Opened " + root.string() + " recursively — " + std::to_string(tracks.size()) + " tracks indexed.";
  }, compact_button_option);
  std::vector<std::string> library_root_labels;
  int library_root_selected = 0;
  auto sync_library_root_labels = [&] {
    library_root_labels.clear();
    for (const auto& folder : library_roots) library_root_labels.push_back(folder.string());
    if (library_root_labels.empty()) library_root_labels.push_back("No library directories configured");
    library_root_selected = std::clamp(library_root_selected, 0, static_cast<int>(library_root_labels.size()) - 1);
  };
  sync_library_root_labels();
  auto add_folder_button = Button("Add directory", [&] {
    const std::filesystem::path requested(folder_text);
    if (requested.empty() || !std::filesystem::is_directory(requested)) { status = "Directory unavailable: " + requested.string(); return; }
    const auto normalized = std::filesystem::weakly_canonical(requested);
    if (std::ranges::any_of(library_roots, [&](const auto& folder) { return std::filesystem::weakly_canonical(folder) == normalized; })) { status = "Directory is already in the Library list."; return; }
    library_roots.push_back(normalized); root = normalized; recursive = true; sync_library_root_labels(); refresh(); persist_preferences();
    status = "Added " + normalized.string() + " recursively — " + std::to_string(tracks.size()) + " tracks indexed.";
  }, compact_button_option);
  auto choose_folder_button = Button("Choose folder…", [&] {
#ifdef _WIN32
    if (const auto chosen = choose_audio_folder()) {
      folder_text = chosen->string();
      add_folder_button->OnEvent(Event::Return);
    } else {
      status = "Folder selection cancelled; current Library folder is unchanged.";
    }
#else
    status = "Native folder picker is unavailable; edit the path and press Add directory.";
#endif
  }, compact_button_option);
  auto remove_folder_button = Button("Remove selected directory", [&] {
    if (library_roots.empty() || library_root_selected < 0 || library_root_selected >= static_cast<int>(library_roots.size())) { status = "No library directory is configured."; return; }
    const auto removed = library_roots[static_cast<std::size_t>(library_root_selected)];
    library_roots.erase(library_roots.begin() + library_root_selected);
    if (library_roots.empty()) library_roots.push_back(std::filesystem::current_path());
    library_root_selected = std::min(library_root_selected, static_cast<int>(library_roots.size()) - 1);
    root = library_roots[static_cast<std::size_t>(library_root_selected)]; folder_text = root.string(); sync_library_root_labels(); refresh(); persist_preferences();
    status = "Removed " + removed.string() + "; Library now indexes " + std::to_string(tracks.size()) + " tracks.";
  }, compact_button_option);
  RadioboxOption library_root_option;
  library_root_option.on_change = [&] {
    if (library_root_selected < 0 || library_root_selected >= static_cast<int>(library_roots.size())) return;
    root = library_roots[static_cast<std::size_t>(library_root_selected)];
    folder_text = root.string();
    refresh();
    status = "Selected Library directory: " + root.string();
  };
  auto library_root_picker = Radiobox(&library_root_labels, &library_root_selected, library_root_option);
  auto play_button = Button("Play", [&] {
    if (selected < 0 || selected >= static_cast<int>(tracks.size())) {
      status = "Select a supported audio file before starting playback.";
      return;
    }
#if defined(_WIN32) || defined(__APPLE__) || defined(BEATGLASS_HAS_PIPEWIRE)
#ifdef _WIN32
    if (!playback_devices.empty() && playback_device_selected >= 0 && playback_device_selected < static_cast<int>(playback_devices.size())) playback.set_device(playback_devices[static_cast<std::size_t>(playback_device_selected)].id);
#endif
    if (playback.state() == beatglass::PlaybackState::paused) {
      playback.resume();
      status = "Resuming " + tracks[selected].title + " through the selected output.";
    } else {
      playback.play(tracks[selected].path);
      status = "Playing " + tracks[selected].title + " through the selected output.";
    }
#else
    status = "Native playback is unavailable in this build; analysis remains available.";
#endif
  }, compact_button_option);
  auto pause_button = Button("Pause", [&] {
#if defined(_WIN32) || defined(__APPLE__) || defined(BEATGLASS_HAS_PIPEWIRE)
    playback.pause();
    status = "Playback pause requested.";
#else
    if (playback.pause()) status = "Playback paused.";
#endif
  }, compact_button_option);
  auto stop_button = Button("Stop", [&] {
    playback.stop();
    status = "Playback stopped.";
  }, compact_button_option);
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
    result.reset(); waveform.clear(); beat_frames.clear(); key.reset(); chord.reset(); tonal_history.clear();
    if (const auto cached = completed_analyses.find(tracks[selected].path); cached != completed_analyses.end()) {
      result = cached->second.result;
      waveform = cached->second.waveform;
      beat_frames = cached->second.beat_frames;
      key = cached->second.key; chord = cached->second.chord;
      tonal_history = cached->second.tonal_history;
    }
    status = "Selected " + tracks[selected].title + " — press Enter to analyze or Space to play." +
        (cancelling ? " Cancelling analysis because the selected track changed." : "");
  };
  auto previous_button = Button("Prev", [&] { select_relative(-1); }, compact_button_option);
  auto next_button = Button("Next", [&] { select_relative(1); }, compact_button_option);
  auto seek_back_button = Button("−", [&] {
#if defined(_WIN32) || defined(__APPLE__) || defined(BEATGLASS_HAS_PIPEWIRE)
    playback.seek_seconds(-seek_step());
    status = "Playback seek backward requested.";
#else
    status = "Native playback seek is unavailable in this build.";
#endif
  }, compact_button_option);
  auto seek_forward_button = Button("+", [&] {
#if defined(_WIN32) || defined(__APPLE__) || defined(BEATGLASS_HAS_PIPEWIRE)
    playback.seek_seconds(seek_step());
    status = "Playback seek forward requested.";
#else
    status = "Native playback seek is unavailable in this build.";
#endif
  }, compact_button_option);
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
    result.reset(); waveform.clear(); beat_frames.clear(); key.reset(); chord.reset(); tonal_history.clear();
    if (const auto cached = completed_analyses.find(tracks[selected].path); cached != completed_analyses.end()) {
      result = cached->second.result;
      waveform = cached->second.waveform;
      beat_frames = cached->second.beat_frames;
      key = cached->second.key; chord = cached->second.chord;
      tonal_history = cached->second.tonal_history;
    }
    status = "Selected " + tracks[selected].title + " — press Enter to analyze or Space to play." +
        (cancelling ? " Cancelling analysis because the selected track changed." : "");
  };
  auto library_menu = Menu(&library_labels, &library_visible_selected, library_menu_option);
  auto library_controls = Container::Vertical(Components{Container::Horizontal(Components{folder_input, choose_folder_button, open_folder_button, recursive_checkbox, sort_button, refresh_button}), Container::Horizontal(Components{search_bar, previous_button, next_button, analyze_selected_button, analyze_all_button, retry_analysis_button,
#ifdef BEATGLASS_CLI_LIBRARY
    recover_persisted_button,
#endif
    play_button, pause_button, stop_button, cancel_analysis_button})});
  auto library_container = Container::Vertical(Components{library_controls, library_menu});
  auto library = Renderer(library_container, [&] {
    Elements rows;
    const auto query = lower(filter);
    library_labels.clear(); library_indices.clear();
    for (int i = 0; i < static_cast<int>(tracks.size()); ++i) {
      const auto searchable = lower(tracks[i].title + " " + tracks[i].path.filename().string() + " " + tracks[i].path.string());
      if (!query.empty() && searchable.find(query) == std::string::npos) continue;
      auto label = tracks[i].title + "  •  " + tracks[i].path.filename().string();
      if (const auto failed = analysis_errors.find(tracks[i].path); failed != analysis_errors.end()) label += failed->second.find("source changed") != std::string::npos ? "  [stale]" : "  [failed]";
      else if (const auto analyzed = completed_analyses.find(tracks[i].path); analyzed != completed_analyses.end()) {
        label += analyzed->second.from_persistent_cache ? "  [cached]" : "  [analyzed]";
        label += "  " + format_bpm(analyzed->second.result.bpm) + " BPM";
        if (analyzed->second.key) label += "  key " + *analyzed->second.key;
      }
      if (tracks[i].metadata) label += "  " + format_duration(*tracks[i].metadata);
      library_labels.push_back(std::move(label)); library_indices.push_back(i);
      if (i == selected) library_visible_selected = static_cast<int>(library_labels.size()) - 1;
    }
    if (library_labels.empty()) rows.push_back(text("No supported audio files in this folder.") | color(Color::GrayLight));
    else rows.push_back(library_menu->Render() | vscroll_indicator | frame);
    const auto analyzed_count = std::count_if(library_indices.begin(), library_indices.end(), [&](const auto index) { return completed_analyses.contains(tracks[index].path); });
    const auto failed_count = std::count_if(library_indices.begin(), library_indices.end(), [&](const auto index) { return analysis_errors.contains(tracks[index].path); });
    Elements summary_parts = {text(std::to_string(library_indices.size())) | bold | color(Color::Cyan), text(" visible  •  "), text(std::to_string(analyzed_count)) | bold | color(Color::GreenLight), text(" analyzed  •  "), text(std::to_string(failed_count)) | bold | color(failed_count ? Color::RedLight : Color::GrayLight), text(" failed"), text("  •  "), text(std::to_string(library_cache_hits)) | bold | color(library_cache_hits ? Color::MagentaLight : Color::GrayLight), text(" cache")};
#ifdef BEATGLASS_CLI_LIBRARY
    if (library_retryable_count != 0) summary_parts.insert(summary_parts.end(), {text("  •  "), text(std::to_string(library_retryable_count)) | bold | color(Color::YellowLight), text(" retry")});
#endif
    auto library_summary = hbox(std::move(summary_parts)) | italic;
    const auto name = selected >= 0 ? tracks[selected].title : "No track selected";
    const auto duration = result ? format_duration(result->metadata)
        : selected >= 0 && selected < static_cast<int>(tracks.size()) && tracks[selected].metadata
            ? format_duration(*tracks[selected].metadata) : "—";
    const auto analysis_display = result ? present_analysis_view(AnalysisView{*result, waveform, beat_frames, key, chord, tonal_history, false}) : beatglass::PresentationState{};
    Elements cards = {metric("TEMPO", result ? analysis_display.tempo + " BPM" : "—", Color::GreenLight), metric("BEATS", result ? std::to_string(analysis_display.beat_count) : "—", Color::YellowLight), metric("STATE", result ? analysis_display.status : "waiting", Color::Cyan)};
    const auto progress_total = analysis_total.load(std::memory_order_acquire);
    const auto progress_frames = analysis_frames.load(std::memory_order_acquire);
    const auto progress = progress_total ? std::min<std::uint64_t>(100, progress_frames * 100 / progress_total) : 0;
    const auto completed = batch_completed.load(std::memory_order_acquire);
    const auto total = batch_total.load(std::memory_order_acquire);
    const auto batch_label = total > 1 ? "  [" + std::to_string(completed) + "/" + std::to_string(total) + " tracks]" : "";
    auto status_view = busy ? vbox({hbox({spinner(6, static_cast<size_t>(spinner_frame)), text(" "), text(status + batch_label)}) | color(Color::YellowLight), progress_total ? progress_meter(progress) : text("Preparing decoder…") | color(Color::GrayLight) | italic}) | bgcolor(Color::RGB(43, 36, 16)) : text(status) | color(Color::GrayLight) | italic;
    const auto playback_label = playback.state() == beatglass::PlaybackState::playing ? "playing" : playback.state() == beatglass::PlaybackState::paused ? "paused" : "stopped";
    const auto playback_status =
#if defined(_WIN32) || defined(__APPLE__) || defined(BEATGLASS_HAS_PIPEWIRE)
        playback.status();
#else
        std::string{};
#endif
    std::string playback_position = "—";
    float playback_fraction = -1.0f;
#if defined(_WIN32) || defined(__APPLE__) || defined(BEATGLASS_HAS_PIPEWIRE)
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
    const auto format = result ? result->metadata.format : selected >= 0 && selected < static_cast<int>(tracks.size()) && tracks[selected].metadata ? tracks[selected].metadata->format : "—";
    const auto sample_rate = result ? result->metadata.sample_rate : selected >= 0 && selected < static_cast<int>(tracks.size()) && tracks[selected].metadata ? tracks[selected].metadata->sample_rate : 0;
    const auto channels = result ? result->metadata.channels : selected >= 0 && selected < static_cast<int>(tracks.size()) && tracks[selected].metadata ? tracks[selected].metadata->channels : 0;
    auto detail = vbox({text("TRACK ANALYSIS") | bold | color(Color::Cyan), text(name) | color(Color::White), selected >= 0 && selected < static_cast<int>(tracks.size()) ? text("Source: " + tracks[selected].path.string()) | color(Color::GrayLight) | italic : text("Source: —") | color(Color::GrayLight) | italic, separator(), hbox(cards) | size(HEIGHT, LESS_THAN, 6), separator(), waveform_chart(result ? waveform : std::vector<float>{}, result ? beat_frames : std::vector<std::uint64_t>{}, result ? result->metadata.frames : 0, playback_fraction) | size(HEIGHT, EQUAL, 5), hbox({text("Waveform: ") | color(Color::GrayLight) | bold, text("PCM amplitude") | color(Color::GreenLight), text("  •  "), text("detected beat frames") | color(Color::YellowLight), text("  •  "), text("playback cursor") | color(Color::Cyan)}), separator(), text(format == "—" ? "Select a supported track to inspect it" : "Format: " + format) | bold, text(sample_rate ? "Sample rate: " + std::to_string(sample_rate) + " Hz   Channels: " + std::to_string(channels) + "   Duration: " + duration : "Real decoder and aubio analysis will populate this view."), text("Key: " + (result ? analysis_display.key : std::string("—")) + "   Chord: " + (result ? analysis_display.chord : std::string("—"))) | color(result && (analysis_display.key != "—" || analysis_display.chord != "—") ? Color::GreenLight : Color::GrayLight), text("Tonal history: " + (result ? join_history(analysis_display.tonal_history) : std::string("—"))) | color(result && !analysis_display.tonal_history.empty() ? Color::GreenLight : Color::GrayLight), text("Playback: " + std::string(playback_label) + "  position " + playback_position + "  epoch " + std::to_string(playback.epoch())), text(playback_status), separator(), status_view});
    const auto library_navigation = hbox({action(previous_button->Render(), Color::GrayLight), text(" "), action(next_button->Render(), Color::GrayLight), text(" "), action(analyze_selected_button->Render(), Color::GreenLight), text(" "), action(analyze_all_button->Render(), Color::GreenLight)});
    const auto library_recovery = hbox({action(retry_analysis_button->Render(), Color::YellowLight),
#ifdef BEATGLASS_CLI_LIBRARY
      text(" "), action(recover_persisted_button->Render(), Color::MagentaLight),
#endif
      text(" "), action(play_button->Render(), Color::Cyan), text(" "), action(pause_button->Render(), Color::YellowLight), text(" "), action(stop_button->Render(), Color::RedLight), text(" "), action(cancel_analysis_button->Render(), Color::RedLight)});
    const auto library_seek = hbox({action(seek_back_button->Render(), Color::GrayLight), text(" "), action(seek_forward_button->Render(), Color::GrayLight)});
    const auto directory_actions = hbox({text("Directory: ") | italic, folder_input->Render() | flex, text(" "), action(choose_folder_button->Render(), Color::Cyan), text(" "), action(add_folder_button->Render(), Color::GreenLight), text(" "), action(open_folder_button->Render(), Color::Cyan), text(" "), recursive_checkbox->Render(), text(" "), action(sort_button->Render(), Color::YellowLight), text(" "), action(refresh_button->Render(), Color::GreenLight)});
    const auto summary_search = hbox({library_summary | flex, text("  "), search_bar->Render()});
    // Keep the command area to one dense row.  The Library list and analysis
    // pane are the primary content; controls should not consume several rows
    // before either one is visible.
    const auto command_strip = hbox({library_navigation, text("  "), library_recovery, text("  "), library_seek}) | frame;
    return vbox({text("LIBRARY") | bold | underlined | color(Color::Cyan), separator(), directory_actions, summary_search, command_strip, separator(), hbox({vbox(rows) | flex, separator(), detail | flex}) | flex, separator(), text("↑↓ / mouse select   Enter/analyze selected   Analyze all visible (a)   Sort title/path (s)   Refresh (r)   Add directory (o)   Tab switch tab   q quit") | color(Color::GrayDark) | italic}) | borderRounded | bgcolor(Color::RGB(10, 15, 22)) | flex;
  });
  std::string live_button_label = " Start monitoring ";
  auto set_live_presentation_state = [&](beatglass::TrackingState state) {
    beatglass::Snapshot snapshot;
    snapshot.state = state;
    live_presentation = beatglass::present_snapshot(snapshot);
  };
  auto clear_live_evidence = [&] {
    live_bpm.reset();
    live_key.reset();
    live_chord.reset();
    live_tonal_candidate_key.clear();
    live_tonal_candidate_chord.clear();
    live_tonal_candidate_streak = 0;
    live_tonal_history.clear();
    live_tracking_state = beatglass::TrackingState::stopped;
    live_sample_rate = 0;
    live_observed_frames = 0;
    live_discontinuities = 0;
    live_beats = 0;
    live_signal_confidence = 0.0f;
    live_waveform.assign(live_waveform.size(), 0.0f);
    live_spectrum.assign(live_spectrum.size(), 0.0f);
    live_spectrum_history.clear();
    live_tempo_history.clear();
    live_last_packet = {};
    live_has_packet = false;
    live_visualization.reset();
    live_visualization_frame.reset();
    live_visualization_presentation.clear();
    live_visualization_revision = 0;
    set_live_presentation_state(beatglass::TrackingState::stopped);
  };
  // Initialize the shared presentation model before the first render. Snapshot
  // mode and the first interactive frame do not receive a capture event, so
  // leaving this default-constructed would produce blank status/key/chord
  // fields instead of the GUI-equivalent stopped/unknown state.
  set_live_presentation_state(beatglass::TrackingState::stopped);
  auto toggle_live = [&] {
#ifdef _WIN32
    if (live_session && live_session->state() != beatglass::SourceState::stopped) {
      live_session->stop();
      clear_live_evidence();
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
      set_live_presentation_state(beatglass::TrackingState::reacquiring);
      live_last_packet = std::chrono::steady_clock::now();
      live_has_packet = false;
      live_status = "Starting capture…";
      live_button_label = " Stop monitoring ";
    }
    else live_status = "Capture could not start; inspect endpoint permissions or device state.";
#elif defined(BEATGLASS_HAS_PIPEWIRE) || defined(__APPLE__)
    if (live_pipewire && live_pipewire->selected_active()) {
      live_pipewire->stop();
      clear_live_evidence();
      live_status = "Capture stopped.";
      live_button_label = " Start monitoring ";
      return;
    }
    if (!live_pipewire || live_endpoint_ids.empty()) { live_status =
#ifdef __APPLE__
      "No macOS output source is available."
#else
      "No PipeWire output sink is available."
#endif
      ;
      return; }
    if (live_selected < 0 || live_selected >= static_cast<int>(live_endpoint_ids.size())) { live_status =
#ifdef __APPLE__
      "Select an available macOS output source first."
#else
      "Select an available output sink first."
#endif
      ;
      return; }
    if (live_pipewire->start_stream(live_endpoint_ids[static_cast<std::size_t>(live_selected)], ++live_pipewire_epoch, 48000, 2)) {
      set_live_presentation_state(beatglass::TrackingState::reacquiring);
      live_last_packet = std::chrono::steady_clock::now();
      live_has_packet = false;
      live_status =
#ifdef __APPLE__
        "Starting CoreAudio output tap…";
#else
        "Starting PipeWire monitor…";
#endif
      live_button_label = " Stop monitoring ";
    } else live_status =
#ifdef __APPLE__
      "CoreAudio output tap could not start; check recording permission.";
#else
      "PipeWire monitor could not start; refresh sinks and retry.";
#endif
#else
    live_status = "Native loopback capture is unavailable in this build.";
#endif
  };
  auto live_button = Button(&live_button_label, toggle_live, compact_button_option);
  auto refresh_live_button = Button("Refresh sinks", [&] {
#ifdef _WIN32
    const auto stopped_for_refresh = live_session && live_session->state() != beatglass::SourceState::stopped;
    if (stopped_for_refresh) {
      live_session->stop();
      clear_live_evidence();
      live_button_label = " Start monitoring ";
      live_status = "Capture stopped while render sinks were refreshed.";
    }
    live_endpoint_snapshot = beatglass::windows::enumerate_render_endpoints();
    live_error = live_endpoint_snapshot.error;
    live_endpoints.clear(); live_endpoint_ids.clear(); live_selected = 0;
    bool refreshed_identity_found = false;
    for (const auto& endpoint : live_endpoint_snapshot.endpoints) {
      live_endpoints.push_back(endpoint.name + (endpoint.is_default ? " (default)" : ""));
      live_endpoint_ids.push_back(endpoint.id);
      if (endpoint.id == live_endpoint_preference) { live_selected = static_cast<int>(live_endpoint_ids.size()) - 1; refreshed_identity_found = true; }
      else if (!refreshed_identity_found && (endpoint.name == live_endpoint_name_preference || (live_endpoint_preference.empty() && live_endpoint_name_preference.empty() && endpoint.is_default))) live_selected = static_cast<int>(live_endpoint_ids.size()) - 1;
    }
    if (!stopped_for_refresh) live_status = std::to_string(live_endpoints.size()) + " render sinks discovered.";
#elif defined(__APPLE__)
    const auto stopped_for_refresh = live_pipewire && live_pipewire->selected_active();
    if (stopped_for_refresh) { live_pipewire->stop(); clear_live_evidence(); live_button_label = " Start monitoring "; }
    const auto refreshed = live_pipewire ? live_pipewire->enumerate() : beatglass::EnumerationResult{};
    live_error = refreshed.diagnostic; live_endpoints.clear(); live_endpoint_ids.clear(); live_selected = 0;
    bool refreshed_identity_found = false;
    for (const auto& endpoint : refreshed.sources) { live_endpoints.push_back(endpoint.name); live_endpoint_ids.push_back(endpoint.id); if (endpoint.id == live_endpoint_preference) { live_selected = static_cast<int>(live_endpoint_ids.size()) - 1; refreshed_identity_found = true; } else if (!refreshed_identity_found && endpoint.name == live_endpoint_name_preference) live_selected = static_cast<int>(live_endpoint_ids.size()) - 1; }
    live_status = std::to_string(live_endpoints.size()) + " CoreAudio output sources discovered.";
#elif defined(BEATGLASS_HAS_PIPEWIRE)
    const auto stopped_for_refresh = live_pipewire && live_pipewire->selected_active();
    if (stopped_for_refresh) { live_pipewire->stop(); clear_live_evidence(); live_button_label = " Start monitoring "; }
    const auto refreshed = beatglass::pipewire::discover_sinks(std::chrono::milliseconds(500));
    live_error = refreshed.error;
    live_endpoints.clear(); live_endpoint_ids.clear(); live_selected = 0;
    bool refreshed_identity_found = false;
    for (const auto& endpoint : refreshed.sinks) {
      live_endpoints.push_back(endpoint.name.empty() ? endpoint.description : endpoint.name);
      live_endpoint_ids.push_back(endpoint.serial);
      if (endpoint.serial == live_endpoint_preference) { live_selected = static_cast<int>(live_endpoint_ids.size()) - 1; refreshed_identity_found = true; }
      else if (!refreshed_identity_found && (endpoint.name == live_endpoint_name_preference || endpoint.description == live_endpoint_name_preference)) live_selected = static_cast<int>(live_endpoint_ids.size()) - 1;
    }
    live_status = std::to_string(live_endpoints.size()) + " PipeWire output sinks discovered.";
#else
    live_status = "Native sink discovery is unavailable in this build.";
#endif
  }, compact_button_option);
  auto reset_live_button = Button("Reset / reacquire", [&] {
#ifdef _WIN32
    if (live_session && live_session->state() != beatglass::SourceState::stopped) live_session->stop();
    live_replay = std::make_unique<beatglass::CausalTempoReplay>();
#ifdef BEATGLASS_CLI_MUSIC_REPLAY
    live_music_replay = std::make_unique<beatglass::CausalMusicReplay>();
#endif
    live_key.reset(); live_chord.reset(); live_tonal_history.clear();
    clear_live_evidence();
    live_tracking_state = beatglass::TrackingState::reacquiring;
    set_live_presentation_state(beatglass::TrackingState::reacquiring);
    live_button_label = " Start monitoring ";
    live_status = "Analysis reset; start monitoring to reacquire.";
#elif defined(BEATGLASS_HAS_PIPEWIRE) || defined(__APPLE__)
    if (live_pipewire && live_pipewire->selected_active()) live_pipewire->stop();
    live_replay = std::make_unique<beatglass::CausalTempoReplay>();
#ifdef BEATGLASS_CLI_MUSIC_REPLAY
    live_music_replay = std::make_unique<beatglass::CausalMusicReplay>();
    live_key.reset(); live_chord.reset(); live_tonal_history.clear();
#endif
    clear_live_evidence();
    live_tracking_state = beatglass::TrackingState::reacquiring;
    set_live_presentation_state(beatglass::TrackingState::reacquiring);
    live_button_label = " Start monitoring ";
    live_status = "Analysis reset; start monitoring to reacquire.";
#else
    live_status = "Native loopback capture is unavailable in this build.";
#endif
  }, compact_button_option);
  auto set_live_multiplier = [&](float multiplier) {
    live_bpm_multiplier = multiplier;
    live_status = std::string("Displayed tempo correction set to ") + (multiplier < 1.0f ? "half" : multiplier > 1.0f ? "double" : "normal") + " BPM.";
    persist_preferences();
  };
  auto half_bpm_button = Button("½ BPM", [&] { set_live_multiplier(0.5f); }, compact_button_option);
  auto normal_bpm_button = Button("Normal BPM", [&] { set_live_multiplier(1.0f); }, compact_button_option);
  auto double_bpm_button = Button("2× BPM", [&] { set_live_multiplier(2.0f); }, compact_button_option);
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
#if defined(BEATGLASS_HAS_PIPEWIRE) || defined(__APPLE__)
    if (live_pipewire && live_pipewire->selected_active()) {
      live_pipewire->stop();
      clear_live_evidence();
      live_tracking_state = beatglass::TrackingState::stopped;
      live_button_label = " Start monitoring ";
      live_status = "Capture stopped because the selected sink changed.";
      return;
    }
#endif
    live_status = "Selected sink: " + live_endpoints[static_cast<std::size_t>(live_selected)];
    persist_preferences();
  };
  auto endpoint_picker = Radiobox(&live_endpoints, &live_selected, endpoint_picker_option);
  auto live = Renderer(Container::Vertical(Components{endpoint_picker, Container::Horizontal(Components{live_button, refresh_live_button, reset_live_button, half_bpm_button, normal_bpm_button, double_bpm_button})}), [&] {
    Elements endpoints;
    for (std::size_t index = 0; index < live_endpoints.size(); ++index) {
      auto label = live_endpoints[index];
      endpoints.push_back(text("●  " + label) | color(Color::YellowLight));
    }
    if (endpoints.empty()) endpoints.push_back(text("No active output sinks discovered.") | color(Color::GrayLight));
    const auto live_state = live_presentation.status;
    const auto live_starting =
#ifdef _WIN32
        live_session && live_session->state() == beatglass::SourceState::starting;
#else
        false;
#endif
    const auto displayed_live_status = live_presentation.stale ? live_status + "  [stale evidence — reacquiring]" : live_status;
    auto live_status_view = live_starting ? hbox({spinner(6, static_cast<std::size_t>(spinner_frame)), text(" "), text(displayed_live_status)}) | color(Color::YellowLight) | bgcolor(Color::RGB(43, 36, 16)) : text(displayed_live_status) | color(Color::GrayLight) | italic;
    const auto corrected_bpm = live_presentation.stale || !live_bpm ? std::optional<float>{} : std::optional<float>{*live_bpm * live_bpm_multiplier};
    const auto live_key_label = live_presentation.stale ? std::string("—") : (!live_presentation.key.empty() && live_presentation.key != "—") ? live_presentation.key : live_key.value_or("—");
    const auto live_chord_label = live_presentation.stale ? std::string("—") : (!live_presentation.chord.empty() && live_presentation.chord != "—") ? live_presentation.chord : live_chord.value_or("—");
    const auto live_tonal_label = live_presentation.stale ? std::string("stale evidence — waiting for fresh tonal windows") : live_presentation.tonal_history.empty() ? (live_tonal_history.empty() ? std::string("waiting for tonal windows") : join_history(live_tonal_history)) : join_history(live_presentation.tonal_history);
    const auto multiplier_label = live_bpm_multiplier < 1.0f ? "½×" : live_bpm_multiplier > 1.0f ? "2×" : "1×";
    const auto shared_confidence = live_presentation.stale
        ? std::optional<float>{}
        : live_presentation.confidence ? live_presentation.confidence : std::optional<float>{live_signal_confidence};
    const auto shared_observed_frames = live_presentation.observed_frames ? live_presentation.observed_frames : live_observed_frames;
    const auto shared_beat_count = live_presentation.beat_count ? live_presentation.beat_count : live_beats;
    const auto shared_analysis_delay = live_presentation.analysis_delay_frames;
    const auto spectrum_view = vbox({text("Spectrum") | bold | color(Color::MagentaLight), spectrum_chart(live_spectrum), text("FFT history") | bold | color(Color::MagentaLight), spectrum_history_chart(live_spectrum_history)});
    const auto waveform_view = vbox({text("Waveform") | bold | color(Color::GreenLight), waveform_chart(live_waveform, {}, shared_observed_frames)});
    std::string beat_grid(32, '.');
    for (const auto frame : live_presentation.beat_frames) {
      const auto position = shared_observed_frames ? std::min<std::size_t>(beat_grid.size() - 1, static_cast<std::size_t>(frame * beat_grid.size() / shared_observed_frames)) : beat_grid.size() - 1;
      beat_grid[position] = '*';
    }
    const auto beat_grid_view = vbox({text("Beat grid") | bold | color(Color::YellowLight), text("|" + beat_grid + "|") | color(Color::YellowLight) | bgcolor(Color::RGB(35, 28, 8)), text("Recent beats: " + std::to_string(shared_beat_count)) | color(Color::GrayLight)});
    const auto tonal_history_view = vbox({text("Tonal history") | bold | color(Color::BlueLight), text(live_tonal_label) | color(Color::BlueLight), text("Key: " + live_key_label + "   Chord: " + live_chord_label) | color(Color::White)});
    const auto visualization = (visual_mode == "waveform" ? waveform_view : visual_mode == "spectrum" ? spectrum_view : visual_mode == "beat_grid" ? beat_grid_view : visual_mode == "tonal_history" ? tonal_history_view : vbox({spectrum_view, waveform_view})) | size(HEIGHT, LESS_THAN, 5);
    std::vector<std::string> tempo_labels;
    const auto& displayed_tempo_history = live_presentation.tempo_history.empty() ? live_tempo_history : live_presentation.tempo_history;
    for (const auto bpm : displayed_tempo_history) tempo_labels.push_back(format_bpm(bpm));
    const auto confidence_label = shared_confidence ? std::to_string(static_cast<int>(*shared_confidence * 100.0f)) + "% (RMS/peak health)" : "— (stale evidence)";
    return vbox({text("LIVE") | bold | underlined | color(Color::Cyan), separator(), text("OS AUDIO SINKS") | bold | color(Color::YellowLight), text("Choose a render endpoint before monitoring.") | italic, separator(), endpoint_picker->Render() | size(HEIGHT, LESS_THAN, 4) | frame | vscroll_indicator, live_error.empty() ? text("Endpoint discovery ready.") | color(Color::GreenLight) : text("Endpoint status: " + live_error) | color(Color::YellowLight), separator(), hbox({action(live_button->Render(), Color::GreenLight), text(" "), action(refresh_live_button->Render(), Color::Cyan), text(" "), action(reset_live_button->Render(), Color::YellowLight)}), hbox({text("Tempo interpretation: ") | italic, action(half_bpm_button->Render(), Color::YellowLight), text(" "), action(normal_bpm_button->Render(), Color::GreenLight), text(" "), action(double_bpm_button->Render(), Color::YellowLight), text("  active ") | color(Color::GrayLight), text(multiplier_label) | bold | color(Color::YellowLight)}), live_status_view, separator(), text("LIVE ANALYSIS") | bold | underlined, text("Tempo: " + (corrected_bpm ? format_bpm(corrected_bpm) + " BPM" : "— BPM")) | bold | color(Color::GreenLight), text("Signal: " + (live_sample_rate ? std::to_string(live_sample_rate) + " Hz" : "—") + "   observed " + std::to_string(shared_observed_frames) + " frames   beats " + std::to_string(shared_beat_count) + "   discontinuities " + std::to_string(live_discontinuities)), text("Signal confidence: " + confidence_label) | color(Color::MagentaLight), text("Analysis delay: " + std::to_string(shared_analysis_delay) + " frames") | color(Color::GrayLight), text("Key: " + live_key_label + "    Chord: " + live_chord_label) | italic, text("Tonal history: " + live_tonal_label) | color(Color::GrayLight), text("Capture state: " + std::string(live_state)), text("Tempo history: " + (tempo_labels.empty() ? std::string("waiting for tracking") : join_history(tempo_labels))) | color(Color::YellowLight), visualization, filler(), text("↑↓ choose sink   Tab switch tab   q quit") | color(Color::GrayDark) | italic}) | borderRounded | bgcolor(Color::RGB(10, 15, 22)) | flex;
  });
  auto reduce_motion_checkbox = Checkbox("Reduce motion", &reduce_motion);
  auto set_visual_mode = [&](std::string mode) { visual_mode = std::move(mode); persist_preferences(); };
  auto waveform_mode_button = Button("Waveform", [&] { set_visual_mode("waveform"); }, compact_button_option);
  auto spectrum_mode_button = Button("Spectrum", [&] { set_visual_mode("spectrum"); }, compact_button_option);
  auto combined_mode_button = Button("Combined", [&] { set_visual_mode("combined"); }, compact_button_option);
  auto beat_grid_mode_button = Button("Beat grid", [&] { set_visual_mode("beat_grid"); }, compact_button_option);
  auto tonal_history_mode_button = Button("Tonal history", [&] { set_visual_mode("tonal_history"); }, compact_button_option);
  auto visualization_mode_action = [&](Component button, const std::string& mode, Color accent) {
    auto rendered = button->Render() | color(visual_mode == mode ? Color::White : accent);
    return visual_mode == mode ? rendered | bgcolor(accent) | bold : rendered;
  };
  RadioboxOption playback_device_option;
  playback_device_option.on_change = [&] {
    persist_preferences();
#ifdef _WIN32
    if (!playback_devices.empty() && playback_device_selected >= 0 && playback_device_selected < static_cast<int>(playback_devices.size())) {
      const auto device_id = playback_devices[static_cast<std::size_t>(playback_device_selected)].id;
      if (playback.state() == beatglass::PlaybackState::playing || playback.state() == beatglass::PlaybackState::paused) playback.set_device(device_id);
      status = "Playback output set to " + playback_devices[static_cast<std::size_t>(playback_device_selected)].name + ".";
    }
#else
    status = "Playback output preference saved; this platform uses its native default output.";
#endif
  };
  auto playback_device_picker = Radiobox(&playback_device_labels, &playback_device_selected, playback_device_option);
  auto settings = Renderer(Container::Vertical(Components{reduce_motion_checkbox, seek_step_input, waveform_mode_button, spectrum_mode_button, combined_mode_button, beat_grid_mode_button, tonal_history_mode_button, playback_device_picker}), [&] {
    return vbox({text("SETTINGS") | bold | underlined | color(Color::Cyan), separator(), text("SESSION") | bold | color(Color::YellowLight),
      text("Library directories (recursive)") | bold, library_root_picker->Render() | frame | vscroll_indicator,
      action(remove_folder_button->Render(), Color::RedLight), text("Selected directory: " + root.string()) | color(Color::GrayLight), text("Analysis engine: aubio / file analyzer"),
      text("Tonal engine: provisional FFT fallback; optional Essentia music-replay build provides higher-fidelity causal labels.") | color(Color::GrayLight) | italic,
      text("Preferences persist in the platform Beatglass CLI config."), text("Config: " + preferences_path().string()) | color(Color::GrayLight) | italic, separator(),
      text("PRESENTATION") | bold | underlined, reduce_motion_checkbox->Render(),
      text(reduce_motion ? "Spinner animation paused; status still updates." : "Spinner animation enabled while work is active.") | color(Color::GrayLight) | italic,
      text("PLAYBACK") | bold | underlined, hbox({text("Seek step (1–60 seconds): ") | italic, seek_step_input->Render() | size(WIDTH, LESS_THAN, 8)}),
      text("LIVE VISUALIZATION") | bold | underlined,
      hbox({text("Mode: " + visual_mode + "  ") | italic, visualization_mode_action(waveform_mode_button, "waveform", Color::GreenLight), text(" "), visualization_mode_action(spectrum_mode_button, "spectrum", Color::MagentaLight), text(" "), visualization_mode_action(combined_mode_button, "combined", Color::Cyan), text(" "), visualization_mode_action(beat_grid_mode_button, "beat_grid", Color::YellowLight), text(" "), visualization_mode_action(tonal_history_mode_button, "tonal_history", Color::BlueLight)}),
      text("PLAYBACK OUTPUT") | bold | underlined,
      playback_device_labels.empty() ? text("No waveOut output devices discovered; the native mapper will be used.") | color(Color::YellowLight) : playback_device_picker->Render() | frame | vscroll_indicator,
      separator(), text("OUTPUT SAFETY") | bold, text("Beatglass never writes tags or modifies source audio."),
      text("Live capture requires an explicit platform source."), filler(), text("Tab switch tab   q quit") | color(Color::GrayDark)}) | borderRounded | flex;
  });
  auto tabs = Container::Tab(Components{library, live, settings}, &tab);
  ButtonOption tab_button_option;
  tab_button_option.transform = [](const EntryState& state) {
    auto label = text(state.label) | color(state.focused ? Color::White : Color::GrayLight);
    return state.focused ? label | bold : label;
  };
  auto library_tab = Button("[1] Library", [&] { tab = 0; }, tab_button_option);
  auto live_tab = Button("[2] Live", [&] { tab = 1; }, tab_button_option);
  auto settings_tab = Button("[3] Settings", [&] { tab = 2; }, tab_button_option);
  auto tab_bar = Renderer(Container::Horizontal(Components{library_tab, live_tab, settings_tab}), [&] {
    auto library_render = library_tab->Render();
    auto live_render = live_tab->Render();
    auto settings_render = settings_tab->Render();
    const auto active_tab = [](Element element, Color accent) {
      return element | color(Color::White) | bgcolor(accent) | bold | underlined;
    };
    if (tab == 0) library_render = active_tab(library_render, Color::RGB(25, 92, 112));
    else library_render = library_render | color(Color::GrayLight);
    if (tab == 1) live_render = active_tab(live_render, Color::RGB(25, 102, 68));
    else live_render = live_render | color(Color::GrayLight);
    if (tab == 2) settings_render = active_tab(settings_render, Color::RGB(112, 78, 22));
    else settings_render = settings_render | color(Color::GrayLight);
    return hbox({library_render, text(" "), live_render, text(" "), settings_render}) | center;
  });
  auto body = Renderer(Container::Vertical(Components{tab_bar, tabs}), [&]() -> Element {
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
    if (event == Event::Character("q") || event == Event::Escape) { persist_preferences(); screen.Exit(); return true; }
    if (event == Event::Character("1")) { tab = 0; return true; }
    if (event == Event::Character("2")) { tab = 1; return true; }
    if (event == Event::Character("3")) { tab = 2; return true; }
    if (event == Event::Tab) { tab = (tab + 1) % 3; return true; }
    if (event == Event::Character("o") && tab == 0) { open_folder_button->OnEvent(Event::Return); return true; }
    if (event == Event::Character("a") && tab == 0) { analyze_all_button->OnEvent(Event::Return); return true; }
    if (event == Event::Character("r") && tab == 0) { refresh_button->OnEvent(Event::Return); return true; }
    if (event == Event::Character("s") && tab == 0) { sort_button->OnEvent(Event::Return); return true; }
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
      if (reduce_motion != last_saved_reduce_motion || seek_step_text != last_saved_seek_step || visual_mode != last_saved_visual_mode || playback_device_selected != last_saved_playback_device) {
        persist_preferences();
        last_saved_reduce_motion = reduce_motion;
        last_saved_seek_step = seek_step_text;
        last_saved_visual_mode = visual_mode;
        last_saved_playback_device = playback_device_selected;
      }
      if (!reduce_motion) ++spinner_frame;
#ifdef _WIN32
      if (live_session && live_session->state() != beatglass::SourceState::stopped) {
        live_session->poll();
        if (live_session->state() == beatglass::SourceState::error) {
          std::ostringstream diagnostic;
          diagnostic << "WASAPI loopback failed (HRESULT 0x" << std::hex << static_cast<std::uint32_t>(live_session->error_code()) << "). Refresh sinks and retry.";
          clear_live_evidence();
          live_tracking_state = beatglass::TrackingState::error;
          set_live_presentation_state(beatglass::TrackingState::error);
          live_status = diagnostic.str();
          live_button_label = " Start monitoring ";
        }
        beatglass::windows::LoopbackPacket packet;
        while (live_session->pop(packet)) {
          const auto layout = packet.channels == 1 ? beatglass::ChannelLayout::mono : packet.channels == 2 ? beatglass::ChannelLayout::stereo : beatglass::ChannelLayout::multichannel;
          beatglass::AudioBlock block{{packet.sample_rate, static_cast<std::uint16_t>(packet.channels), layout}, packet.epoch, packet.first_frame, static_cast<std::int64_t>(packet.qpc_100ns) * 100, packet.discontinuity, std::span<const float>(packet.samples.data(), packet.sample_count)};
          try {
            live_sample_rate = packet.sample_rate;
            live_last_packet = std::chrono::steady_clock::now();
            live_has_packet = true;
            const auto packet_channels = std::max<std::uint32_t>(1, packet.channels);
            const auto packet_frames = packet.sample_count / packet_channels;
            live_observed_frames = packet.first_frame + packet_frames;
            if (packet.discontinuity) ++live_discontinuities;
            double energy = 0.0;
            float peak = 0.0f;
            for (std::size_t sample = 0; sample < packet.sample_count; sample += packet_channels) {
              const auto value = std::abs(packet.samples[sample]);
              energy += static_cast<double>(value) * value;
              peak = std::max(peak, value);
            }
            const auto sample_count = std::max<std::size_t>(1, packet_frames);
            const auto rms = static_cast<float>(std::sqrt(energy / static_cast<double>(sample_count)));
            live_signal_confidence = std::clamp(std::max(rms, peak * 0.35f) * 2.0f, 0.0f, 1.0f);
            const auto state = live_replay->process(block);
            live_bpm = state.bpm;
            live_tracking_state = state.state;
            beatglass::Snapshot presentation_snapshot;
            presentation_snapshot.state = state.state;
            presentation_snapshot.observed_through = {state.epoch, state.observed_through};
            presentation_snapshot.analysis_delay_frames = state.observed_through >= state.analyzed_through ? state.observed_through - state.analyzed_through : 0;
            presentation_snapshot.bpm = state.bpm;
            presentation_snapshot.confidence = live_signal_confidence;
            presentation_snapshot.beats.clear();
            for (const auto frame : state.beat_frames) presentation_snapshot.beats.push_back({{state.epoch, frame}, 1.0f});
            live_beats = state.beats;
#ifdef BEATGLASS_CLI_MUSIC_REPLAY
            if (!live_music_replay) live_music_replay = std::make_unique<beatglass::CausalMusicReplay>();
            const auto music_state = live_music_replay->process(block, {}, [&](const beatglass::CausalTonalFrame& tonal) {
              live_key = tonal.rolling_key;
              live_chord = tonal.local_chord;
              if (tonal.local_chord) {
                live_tonal_history.push_back(*tonal.local_chord);
                if (live_tonal_history.size() > 8) live_tonal_history.erase(live_tonal_history.begin());
              }
            });
            live_key = music_state.rolling_key;
            live_chord = music_state.local_chord;
            if (music_state.rolling_key) presentation_snapshot.key_candidates.push_back({*music_state.rolling_key, music_state.rolling_key_strength.value_or(0.0f)});
            if (music_state.tonal_from && music_state.tonal_through) {
              for (const auto& label : live_tonal_history)
                presentation_snapshot.chords.push_back({{state.epoch, *music_state.tonal_from}, {state.epoch, *music_state.tonal_through}, {label, music_state.local_chord_strength.value_or(0.0f)}});
            }
#endif
            if (live_bpm) {
              live_tempo_history.push_back(*live_bpm);
              if (live_tempo_history.size() > 12) live_tempo_history.erase(live_tempo_history.begin());
            }
            presentation_snapshot.tempo_history = live_tempo_history;
            live_presentation = beatglass::present_snapshot(presentation_snapshot);
            std::vector<float> mono(packet_frames);
            for (std::size_t frame = 0; frame < mono.size(); ++frame) {
              float sum = 0.0f;
              for (std::uint32_t channel = 0; channel < packet_channels; ++channel) sum += packet.samples[frame * packet.channels + channel];
              mono[frame] = sum / static_cast<float>(packet_channels);
            }
            for (std::size_t offset = 0; offset < mono.size(); offset += beatglass::VisualizationProcessor::max_input_frames) {
              const auto count = std::min<std::size_t>(beatglass::VisualizationProcessor::max_input_frames, mono.size() - offset);
              if (auto frame = live_visualization.process(packet.epoch, packet.first_frame + offset, packet.sample_rate,
                                                          std::span<const float>(mono.data() + offset, count), packet.discontinuity && offset == 0)) {
                const auto revision = ++live_visualization_revision;
                if (live_visualization_presentation.accept(frame->epoch, revision, *frame)) live_visualization_frame = std::move(frame);
              }
            }
            if (live_visualization_frame) {
              const auto& frame = *live_visualization_frame;
              for (std::size_t bin = 0; bin < live_waveform.size(); ++bin) {
                const auto source = std::min<unsigned>(frame.waveform_size - 1, static_cast<unsigned>(bin * frame.waveform_size / live_waveform.size()));
                live_waveform[bin] = std::max(std::abs(frame.minimum[source]), std::abs(frame.maximum[source]));
              }
              for (std::size_t bin = 0; bin < live_spectrum.size(); ++bin) {
                const auto source = std::min<unsigned>(frame.spectrum_size - 1, static_cast<unsigned>(bin * frame.spectrum_size / live_spectrum.size()));
                live_spectrum[bin] = std::clamp(frame.magnitude[source] * 8.0f, 0.0f, 1.0f);
              }
              live_spectrum_history.push_back(live_spectrum);
              if (live_spectrum_history.size() > 12) live_spectrum_history.erase(live_spectrum_history.begin());
              update_fft_tonal();
              if (live_key) presentation_snapshot.key_candidates.push_back({*live_key, 0.0f});
              if (live_chord) presentation_snapshot.chords.push_back({{state.epoch, state.observed_through > 4096 ? state.observed_through - 4096 : 0}, {state.epoch, state.observed_through}, {*live_chord, 0.0f}});
              live_presentation = beatglass::present_snapshot(presentation_snapshot);
            }
            live_status = "Receiving OS sink audio; analysis is worker-owned and discontinuity-safe.";
          } catch (const std::exception& error) {
            live_tracking_state = beatglass::TrackingState::error;
            beatglass::Snapshot error_snapshot;
            error_snapshot.state = beatglass::TrackingState::error;
            live_presentation = beatglass::present_snapshot(error_snapshot);
            live_status = std::string("Live analysis failed: ") + error.what();
            live_session->stop();
          }
        }
        if (live_has_packet && std::chrono::steady_clock::now() - live_last_packet > std::chrono::milliseconds(1500)) {
          clear_live_evidence();
          live_tracking_state = beatglass::TrackingState::reacquiring;
          set_live_presentation_state(beatglass::TrackingState::reacquiring);
          live_status = "Capture is running but no fresh packets arrived; reacquiring…";
          live_has_packet = false;
        }
      }
      if (live_session && live_session->state() == beatglass::SourceState::stopped) {
        const auto now = std::chrono::steady_clock::now();
        const auto user_stopped = live_status.find("start monitoring") != std::string::npos || live_status == "Capture stopped.";
        if (live_tracking_state == beatglass::TrackingState::reacquiring && !user_stopped &&
            now >= live_next_reacquire && live_selected >= 0 &&
            live_selected < static_cast<int>(live_endpoint_ids.size()) &&
            beatglass::windows::select_render_endpoint(live_endpoint_snapshot, live_endpoint_ids[static_cast<std::size_t>(live_selected)]) == beatglass::windows::SelectionResult::accepted &&
            live_session->start(live_endpoint_ids[static_cast<std::size_t>(live_selected)])) {
          clear_live_evidence();
          live_tracking_state = beatglass::TrackingState::reacquiring;
          set_live_presentation_state(beatglass::TrackingState::reacquiring);
          live_last_packet = now;
          live_has_packet = false;
          live_next_reacquire = now + std::chrono::milliseconds(1500);
          live_button_label = " Stop monitoring ";
          live_status = "WASAPI loopback lost its stream; reacquiring a fresh epoch…";
        } else if (live_tracking_state == beatglass::TrackingState::reacquiring && !user_stopped) {
          live_next_reacquire = now + std::chrono::milliseconds(750);
          live_button_label = " Stop monitoring ";
          live_status = "WASAPI reacquisition failed; retrying…";
        } else {
          live_button_label = " Start monitoring ";
        }
      }
#elif defined(BEATGLASS_HAS_PIPEWIRE) || defined(__APPLE__)
      if (live_pipewire && live_pipewire->selected_active()) {
        beatglass::coreaudio::CaptureBlock packet;
        while (live_pipewire->pop_current(packet)) {
          const auto layout = packet.channels == 1 ? beatglass::ChannelLayout::mono : packet.channels == 2 ? beatglass::ChannelLayout::stereo : beatglass::ChannelLayout::multichannel;
          beatglass::AudioBlock block{{packet.sample_rate, static_cast<std::uint16_t>(packet.channels), layout}, packet.epoch, packet.first_frame, static_cast<std::int64_t>(packet.host_time), packet.discontinuity, std::span<const float>(packet.samples.data(), packet.sample_count)};
          try {
            live_sample_rate = packet.sample_rate;
            live_last_packet = std::chrono::steady_clock::now();
            live_has_packet = true;
            const auto packet_channels = std::max<std::uint32_t>(1, packet.channels);
            const auto packet_frames = packet.sample_count / packet_channels;
            live_observed_frames = packet.first_frame + packet_frames;
            if (packet.discontinuity) ++live_discontinuities;
            double energy = 0.0; float peak = 0.0f;
            for (std::size_t sample = 0; sample < packet.sample_count; sample += packet_channels) { const auto value = std::abs(packet.samples[sample]); energy += static_cast<double>(value) * value; peak = std::max(peak, value); }
            live_signal_confidence = std::clamp(std::max(static_cast<float>(std::sqrt(energy / static_cast<double>(std::max<std::size_t>(1, packet_frames)))), peak * 0.35f) * 2.0f, 0.0f, 1.0f);
            const auto state = live_replay->process(block);
            live_bpm = state.bpm; live_tracking_state = state.state; live_beats = state.beats;
            beatglass::Snapshot presentation_snapshot; presentation_snapshot.state = state.state; presentation_snapshot.observed_through = {state.epoch, state.observed_through}; presentation_snapshot.analysis_delay_frames = state.observed_through >= state.analyzed_through ? state.observed_through - state.analyzed_through : 0; presentation_snapshot.bpm = state.bpm; presentation_snapshot.confidence = live_signal_confidence; for (const auto frame : state.beat_frames) presentation_snapshot.beats.push_back({{state.epoch, frame}, 1.0f});
#ifdef BEATGLASS_CLI_MUSIC_REPLAY
            if (!live_music_replay) live_music_replay = std::make_unique<beatglass::CausalMusicReplay>();
            const auto music_state = live_music_replay->process(block, {}, [&](const beatglass::CausalTonalFrame& tonal) {
              live_key = tonal.rolling_key;
              live_chord = tonal.local_chord;
              if (tonal.local_chord) {
                live_tonal_history.push_back(*tonal.local_chord);
                if (live_tonal_history.size() > 8) live_tonal_history.erase(live_tonal_history.begin());
              }
            });
            live_key = music_state.rolling_key;
            live_chord = music_state.local_chord;
            if (music_state.rolling_key) presentation_snapshot.key_candidates.push_back({*music_state.rolling_key, music_state.rolling_key_strength.value_or(0.0f)});
            if (music_state.tonal_from && music_state.tonal_through) for (const auto& label : live_tonal_history) presentation_snapshot.chords.push_back({{state.epoch, *music_state.tonal_from}, {state.epoch, *music_state.tonal_through}, {label, music_state.local_chord_strength.value_or(0.0f)}});
#endif
            if (live_bpm) { live_tempo_history.push_back(*live_bpm); if (live_tempo_history.size() > 12) live_tempo_history.erase(live_tempo_history.begin()); }
            presentation_snapshot.tempo_history = live_tempo_history;
            live_presentation = beatglass::present_snapshot(presentation_snapshot);
            std::vector<float> mono(packet_frames);
            for (std::size_t frame = 0; frame < mono.size(); ++frame) { float sum = 0.0f; for (std::uint32_t channel = 0; channel < packet_channels; ++channel) sum += packet.samples[frame * packet.channels + channel]; mono[frame] = sum / static_cast<float>(packet_channels); }
            for (std::size_t offset = 0; offset < mono.size(); offset += beatglass::VisualizationProcessor::max_input_frames) { const auto count = std::min<std::size_t>(beatglass::VisualizationProcessor::max_input_frames, mono.size() - offset); if (auto frame = live_visualization.process(packet.epoch, packet.first_frame + offset, packet.sample_rate, std::span<const float>(mono.data() + offset, count), packet.discontinuity && offset == 0)) { const auto revision = ++live_visualization_revision; if (live_visualization_presentation.accept(frame->epoch, revision, *frame)) live_visualization_frame = std::move(frame); } }
            if (live_visualization_frame) { const auto& frame = *live_visualization_frame; for (std::size_t bin = 0; bin < live_waveform.size(); ++bin) { const auto source = std::min<unsigned>(frame.waveform_size - 1, static_cast<unsigned>(bin * frame.waveform_size / live_waveform.size())); live_waveform[bin] = std::max(std::abs(frame.minimum[source]), std::abs(frame.maximum[source])); } for (std::size_t bin = 0; bin < live_spectrum.size(); ++bin) { const auto source = std::min<unsigned>(frame.spectrum_size - 1, static_cast<unsigned>(bin * frame.spectrum_size / live_spectrum.size())); live_spectrum[bin] = std::clamp(frame.magnitude[source] * 8.0f, 0.0f, 1.0f); } live_spectrum_history.push_back(live_spectrum); if (live_spectrum_history.size() > 12) live_spectrum_history.erase(live_spectrum_history.begin()); update_fft_tonal(); if (live_key) presentation_snapshot.key_candidates.push_back({*live_key, 0.0f}); if (live_chord) presentation_snapshot.chords.push_back({{state.epoch, state.observed_through > 4096 ? state.observed_through - 4096 : 0}, {state.epoch, state.observed_through}, {*live_chord, 0.0f}}); live_presentation = beatglass::present_snapshot(presentation_snapshot); }
            live_status =
#ifdef __APPLE__
              "Receiving CoreAudio output-tap audio; analysis is worker-owned and discontinuity-safe.";
#else
              "Receiving PipeWire sink audio; analysis is worker-owned and discontinuity-safe.";
#endif
          } catch (const std::exception& error) { live_tracking_state = beatglass::TrackingState::error; live_status = std::string("Live analysis failed: ") + error.what(); live_pipewire->stop(); }
        }
        const auto pipewire_status = live_pipewire->poll_status();
        // A newly negotiated stream deliberately reports one discontinuity so
        // downstream DSP starts a fresh epoch. That marker is not itself a
        // failed stream; only loss of activity, restart, overflow, or an error
        // requires clearing evidence and reacquiring. MonitorStatus::requires_reacquisition
        // remains the broad contract; this UI path distinguishes its boundary marker.
        if (!pipewire_status.active || pipewire_status.needs_restart || pipewire_status.overflowed || pipewire_status.has_error()) { clear_live_evidence(); live_tracking_state = beatglass::TrackingState::reacquiring; set_live_presentation_state(beatglass::TrackingState::reacquiring); live_status =
#ifdef __APPLE__
          "CoreAudio capture needs reacquisition; restart monitoring for a fresh epoch.";
#else
          "PipeWire capture needs reacquisition; restart monitoring for a fresh epoch.";
#endif
        }
        if (live_has_packet && std::chrono::steady_clock::now() - live_last_packet > std::chrono::milliseconds(1500)) { clear_live_evidence(); live_tracking_state = beatglass::TrackingState::reacquiring; set_live_presentation_state(beatglass::TrackingState::reacquiring); live_status =
#ifdef __APPLE__
          "Capture is running but no fresh CoreAudio packets arrived; reacquiring…";
#else
          "Capture is running but no fresh PipeWire packets arrived; reacquiring…";
#endif
          live_has_packet = false; }
      }
      if (live_pipewire && live_button_label == " Stop monitoring " && !live_pipewire->selected_active() &&
          std::chrono::steady_clock::now() >= live_next_reacquire &&
          live_selected >= 0 && live_selected < static_cast<int>(live_endpoint_ids.size())) {
        clear_live_evidence();
        live_tracking_state = beatglass::TrackingState::reacquiring;
        set_live_presentation_state(beatglass::TrackingState::reacquiring);
        const auto reacquired = live_pipewire->reacquire(live_endpoint_ids[static_cast<std::size_t>(live_selected)], ++live_pipewire_epoch, 48000, 2);
        live_next_reacquire = std::chrono::steady_clock::now() + (reacquired ? std::chrono::milliseconds(1500) : std::chrono::milliseconds(750));
        live_status = reacquired ?
#ifdef __APPLE__
          "CoreAudio output tap lost its stream; reacquiring a fresh epoch…" :
#else
          "PipeWire monitor lost its stream; reacquiring a fresh epoch…" :
#endif
#ifdef __APPLE__
          "CoreAudio reacquisition failed; retrying…";
#else
          "PipeWire reacquisition failed; retrying…";
#endif
      }
#endif
      if (busy && analysis_future.valid() && analysis_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        try { auto analysis = analysis_future.get(); library_cache_hits += analysis.cache_hits; for (auto& entry : analysis.results) { completed_analyses.insert_or_assign(entry.first, std::move(entry.second)); analysis_errors.erase(entry.first); } for (auto& entry : analysis.errors) analysis_errors.insert_or_assign(entry.first, std::move(entry.second));
#ifdef BEATGLASS_CLI_LIBRARY
          if (library_storage) library_retryable_count = library_storage->retryable_runs().size();
#endif
          const auto selection_changed = selected < 0 || selected >= static_cast<int>(tracks.size()) || tracks[selected].path != analysis_path; const auto cached = !selection_changed && completed_analyses.find(analysis_path) != completed_analyses.end(); const auto failed = !selection_changed && analysis_errors.find(analysis_path) != analysis_errors.end(); if (analysis_cancelled.load(std::memory_order_acquire)) { status = "Analysis cancelled; completed track results remain available."; } else if (failed) { result.reset(); waveform.clear(); beat_frames.clear(); key.reset(); chord.reset(); tonal_history.clear(); status = "Analysis failed for " + tracks[selected].title + ": " + analysis_errors.at(analysis_path) + "."; } else if (!cached) { result.reset(); waveform.clear(); beat_frames.clear(); key.reset(); chord.reset(); tonal_history.clear(); status = "Analysis finished for a previous track; select a completed track to view its result."; } else { result = completed_analyses.at(analysis_path).result; waveform = completed_analyses.at(analysis_path).waveform; beat_frames = completed_analyses.at(analysis_path).beat_frames; key = completed_analyses.at(analysis_path).key; chord = completed_analyses.at(analysis_path).chord; tonal_history = completed_analyses.at(analysis_path).tonal_history; status = analysis.errors.empty() ? "Analysis complete — decoder PCM drives the waveform and analyzer drives the estimates." : "Batch complete with " + std::to_string(analysis.errors.size()) + " failed track(s); successful results remain available."; } }
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
    if (tab == 0) { analyze(); if (analysis_future.valid()) { analysis_future.wait(); auto analysis = analysis_future.get(); library_cache_hits += analysis.cache_hits; for (auto& entry : analysis.results) { completed_analyses.insert_or_assign(entry.first, std::move(entry.second)); analysis_errors.erase(entry.first); } for (auto& entry : analysis.errors) analysis_errors.insert_or_assign(entry.first, std::move(entry.second)); if (const auto cached = completed_analyses.find(analysis_path); cached != completed_analyses.end()) { result = cached->second.result; waveform = cached->second.waveform; beat_frames = cached->second.beat_frames; key = cached->second.key; chord = cached->second.chord; tonal_history = cached->second.tonal_history; } status = analysis.errors.empty() ? "Analysis complete — decoder PCM drives the waveform and analyzer drives the estimates." : "Analysis finished with " + std::to_string(analysis.errors.size()) + " failed track(s)."; busy = false; } }
    auto rendered = Screen::Create(Dimension::Fixed(120), Dimension::Fixed(36));
    Render(rendered, root_component->Render());
    // FTXUI's snapshot string already carries carriage returns on Windows;
    // text-mode stdout adds another one when translating newlines. Remove the
    // embedded CR so redirected snapshots keep normal one-row-per-line spacing.
    auto snapshot_output = rendered.ToString();
    for (std::size_t position = 0; (position = snapshot_output.find("\r\n", position)) != std::string::npos;) {
      snapshot_output.erase(position, 1);
      ++position;
    }
    std::cout << snapshot_output;
    return 0;
  }
  std::atomic_bool running{true};
  std::thread ticker([&] { while (running.load()) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); screen.PostEvent(Event::Custom); } });
  screen.Loop(root_component);
  running.store(false);
  ticker.join();
  return 0;
}
