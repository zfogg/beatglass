// Provisional private PipeWire monitor; not yet connected to AudioSource.
// The control thread owns this object; one worker may consume its queue.
#include "beatglass/capture/detail/pipewire_monitor.hpp"
#include <pipewire/pipewire.h>
#include <spa/buffer/meta.h>
#include <spa/param/audio/format-utils.h>
#include <spa/pod/builder.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <limits>
#include <string>
#include <thread>
#include "beatglass/core/audio_queue.hpp"

namespace beatglass::pipewire {
static_assert(std::atomic<MonitorState>::is_always_lock_free);
static_assert(std::atomic<int>::is_always_lock_free);
static_assert(std::atomic<bool>::is_always_lock_free);
struct Monitor::Impl {
public:
  explicit Impl(std::uint64_t epoch) : ingress_(epoch), epoch_(epoch) {}
  ~Impl() { stop(); }

  // The serial must have come from an explicit sink selection. Epochs are
  // assigned externally and must never be reused while old data is in flight.
  bool start(const Sink& sink) {
    const auto& serial = sink.serial;
    if (started_ || state_.load() != MonitorState::stopped || serial.empty() || sink.name.empty() || !epoch_ ||
        !std::all_of(serial.begin(), serial.end(), [](unsigned char c) { return c >= '0' && c <= '9'; })) return false;
    started_ = true;
    const auto discovered = discover_sinks(std::chrono::seconds(2));
    const auto match = std::count_if(discovered.sinks.begin(), discovered.sinks.end(), [&](const Sink& item) {
      return item.serial == sink.serial && item.name == sink.name;
    });
    const auto same_name = std::count_if(discovered.sinks.begin(), discovered.sinks.end(), [&](const Sink& item) {
      return item.name == sink.name;
    });
    if (!discovered.error.empty() || match != 1 || same_name != 1) {
      state_.store(MonitorState::error, std::memory_order_release);
      return false;
    }
    pw_init(nullptr, nullptr);
    loop_ = pw_main_loop_new(nullptr);
    if (!loop_) { state_.store(MonitorState::error); return false; }
    auto* props = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_MEDIA_ROLE, "Music", PW_KEY_TARGET_OBJECT, serial.c_str(), "node.target", sink.name.c_str(),
        PW_KEY_STREAM_CAPTURE_SINK, "true", PW_KEY_NODE_DONT_RECONNECT, "true", "node.dont-fallback", "true", nullptr);
    if (!props) { stop(); state_.store(MonitorState::error); return false; }
    events_.version = PW_VERSION_STREAM_EVENTS;
    events_.state_changed = &state_changed;
    events_.param_changed = &param_changed;
    events_.process = &process;
    stream_ = pw_stream_new_simple(pw_main_loop_get_loop(loop_), "Beatglass output monitor", props, &events_, this);
    if (!stream_) { stop(); state_.store(MonitorState::error); return false; }
    format_valid_ = false;
    state_.store(MonitorState::starting, std::memory_order_release);
    uint8_t storage[1024];
    spa_pod_builder builder = SPA_POD_BUILDER_INIT(storage, sizeof(storage));
    spa_audio_info_raw info{};
    info.format = SPA_AUDIO_FORMAT_F32;
    info.rate = 48000;
    info.channels = 2;
    info.position[0] = SPA_AUDIO_CHANNEL_FL;
    info.position[1] = SPA_AUDIO_CHANNEL_FR;
    const spa_pod* params[] = {spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &info)};
    const auto flags = static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_DONT_RECONNECT);
    if (pw_stream_connect(stream_, PW_DIRECTION_INPUT, PW_ID_ANY, flags, params, 1) < 0) {
      stop(); state_.store(MonitorState::error); return false;
    }
    try { thread_ = std::thread([this] {
      auto* loop = pw_main_loop_get_loop(loop_);
      pw_loop_enter(loop);
      while (!stop_requested_.load(std::memory_order_acquire)) {
        const int result = pw_loop_iterate(loop, 50);
        if (result < 0 && result != -EINTR) { state_.store(MonitorState::error, std::memory_order_release); break; }
      }
      pw_loop_leave(loop);
    }); }
    catch (...) { stop(); state_.store(MonitorState::error); return false; }
    return true;
  }

  // Stop is control-thread-only. The process callback runs on this main loop,
  // never on PipeWire's RT thread; joining the loop quiesces our callbacks.
  void stop() noexcept {
    state_.store(MonitorState::stopped, std::memory_order_release);
    stop_requested_.store(true, std::memory_order_release);
    if (thread_.joinable()) thread_.join();
    if (stream_) { pw_stream_destroy(stream_); stream_ = nullptr; }
    if (loop_) { pw_main_loop_destroy(loop_); loop_ = nullptr; }
    state_.store(MonitorState::stopped, std::memory_order_release);
  }
  MonitorState state() const noexcept { return state_.load(std::memory_order_acquire); }
  // Consumer must invalidate all retained results on any non-streaming state,
  // drain old blocks at stop, and install an epoch before accepting new ones.
  bool pop(MonitorIngress::Block& block) noexcept {
    if (state() != MonitorState::streaming || !ingress_.pop(block)) return false;
    return state() == MonitorState::streaming;
  }
  std::uint64_t dropped_frames() const noexcept { return ingress_.dropped_frames(); }
  int diagnostic_code() const noexcept { return diagnostic_.load(); }

private:
  static void state_changed(void* data, pw_stream_state, pw_stream_state next, const char*) {
    auto& self = *static_cast<Impl*>(data);
    if (self.state() == MonitorState::stopped || self.state() == MonitorState::error || self.state() == MonitorState::interrupted) return;
    if (next == PW_STREAM_STATE_ERROR || next == PW_STREAM_STATE_UNCONNECTED) {
      self.diagnostic_.store(1);
      self.state_.store(MonitorState::error, std::memory_order_release);
    } else if (next == PW_STREAM_STATE_STREAMING && self.format_valid_ && self.state() == MonitorState::starting) {
      self.state_.store(MonitorState::streaming, std::memory_order_release);
    } else if (next == PW_STREAM_STATE_PAUSED && self.state() == MonitorState::streaming) {
      self.state_.store(MonitorState::interrupted, std::memory_order_release);
    }
  }
  static void param_changed(void* data, uint32_t id, const spa_pod* param) {
    auto& self = *static_cast<Impl*>(data);
    if (id != SPA_PARAM_Format) return;
    if (!param && !self.format_valid_) return;
    spa_audio_info_raw info{};
    if (!param || spa_format_audio_raw_parse(param, &info) < 0 || info.format != SPA_AUDIO_FORMAT_F32 ||
        info.rate != 48000 || info.channels != 2 || info.position[0] != SPA_AUDIO_CHANNEL_FL || info.position[1] != SPA_AUDIO_CHANNEL_FR) {
      self.format_valid_ = false;
      self.diagnostic_.store(!param ? 20 : info.format != SPA_AUDIO_FORMAT_F32 ? 21 : info.rate != 48000 ? 22 : info.channels != 2 ? 23 : 24);
      self.state_.store(MonitorState::error, std::memory_order_release);
      return;
    }
    // Renegotiation requires a fresh epoch, even when the format is unchanged.
    if (self.format_valid_) { self.diagnostic_.store(3); self.state_.store(MonitorState::error, std::memory_order_release); return; }
    self.format_valid_ = true;
    if (self.state() == MonitorState::starting && pw_stream_get_state(self.stream_, nullptr) == PW_STREAM_STATE_STREAMING)
      self.state_.store(MonitorState::streaming, std::memory_order_release);
  }
  static void process(void* data) {
    auto& self = *static_cast<Impl*>(data);
    auto* buffer = pw_stream_dequeue_buffer(self.stream_);
    if (!buffer) return;
    auto* spa = buffer->buffer;
    if (self.state() == MonitorState::streaming && spa && spa->n_datas == 1 && spa->datas &&
        spa->datas[0].data && spa->datas[0].chunk && self.format_valid_) {
      auto& d = spa->datas[0];
      auto* header = static_cast<spa_meta_header*>(spa_buffer_find_meta_data(spa, SPA_META_Header, sizeof(spa_meta_header)));
      const bool corrupt = (d.chunk->flags & SPA_CHUNK_FLAG_CORRUPTED) || (header && (header->flags & SPA_META_HEADER_FLAG_CORRUPTED));
      const bool gap = header && (header->flags & (SPA_META_HEADER_FLAG_DISCONT | SPA_META_HEADER_FLAG_GAP));
      auto bytes = std::span<const std::byte>(static_cast<const std::byte*>(d.data), d.maxsize);
      if (self.ingress_.ingest(bytes, d.chunk->offset, d.chunk->size, d.chunk->stride, corrupt, gap) == IngressResult::invalid) {
        self.diagnostic_.store(4);
        self.state_.store(MonitorState::error, std::memory_order_release);
      }
    } else if (self.state() == MonitorState::streaming) {
      self.diagnostic_.store(5);
      self.state_.store(MonitorState::error, std::memory_order_release);
    }
    pw_stream_queue_buffer(self.stream_, buffer);
  }
  pw_main_loop* loop_{};
  pw_stream* stream_{};
  pw_stream_events events_{};
  std::thread thread_;
  std::atomic<MonitorState> state_{MonitorState::stopped};
  std::atomic<int> diagnostic_{0};
  std::atomic<bool> stop_requested_{false};
  MonitorIngress ingress_;
  std::uint64_t epoch_{};
  bool started_{};
  bool format_valid_{}; // control loop thread, after start before thread creation
};
Monitor::Monitor(std::uint64_t epoch) : impl_(std::make_unique<Impl>(epoch)) {}
Monitor::~Monitor() = default;
bool Monitor::start(const Sink& sink) { return impl_->start(sink); }
void Monitor::stop() noexcept { impl_->stop(); }
MonitorState Monitor::state() const noexcept { return impl_->state(); }
bool Monitor::pop(MonitorIngress::Block& block) noexcept { return impl_->pop(block); }
int Monitor::diagnostic_code() const noexcept { return impl_->diagnostic_code(); }
std::uint64_t Monitor::dropped_frames() const noexcept { return impl_->dropped_frames(); }
} // namespace beatglass::pipewire
