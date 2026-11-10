// Linux-only PipeWire integration seam. Native construction remains provisional
// until a Linux host validates SPA negotiation and pw_stream callback timing.
#if defined(BEATGLASS_HAS_PIPEWIRE)
#include "beatglass/capture/detail/pipewire_stream_owner.hpp"
#include <pipewire/pipewire.h>
#include <cerrno>
#include <spa/param/param.h>
#include <spa/param/audio/format-utils.h>
#include <spa/buffer/buffer.h>

namespace beatglass::pipewire::detail {
// This translation unit intentionally isolates PipeWire headers and the future
// pw_stream callback boundary from the portable StreamOwner contract. The
// callback must dequeue only bounded buffers and hand copied float PCM to
// MonitorStream; it must never allocate, block, or touch UI/database state.
static_assert(PW_VERSION_STREAM_EVENTS > 0, "PipeWire stream callback ABI required");
static_assert(SPA_PARAM_Format > 0, "SPA format negotiation required");

class LinuxStreamOwner final : public StreamOwner {
public:
  ~LinuxStreamOwner() override { stop(); }
  bool connect(const Config& config, MonitorStream& handoff) override {
    if (active_ || config.serial.empty() || !config.sample_rate || !config.channels) return false;
    handoff_ = &handoff;
    sample_rate_ = config.sample_rate;
    channels_ = config.channels;
    loop_ = pw_thread_loop_new("beatglass-pipewire", nullptr);
    if (!loop_) return false;
    context_ = pw_context_new(pw_thread_loop_get_loop(loop_), nullptr, 0);
    if (!context_) { stop(); return false; }
    core_ = pw_context_connect(context_, nullptr, 0);
    if (!core_) { stop(); return false; }
    props_ = pw_properties_new(PW_KEY_TARGET_OBJECT, config.serial.c_str(), nullptr);
    if (!props_) { stop(); return false; }
    stream_ = pw_stream_new_simple(pw_thread_loop_get_loop(loop_), "beatglass-monitor", props_, &events, this);
    if (!stream_) { stop(); return false; }
    if (pw_thread_loop_start(loop_) < 0) { stop(); return false; }
    loop_started_ = true;
    std::uint8_t pod_buffer[1024];
    spa_pod_builder builder = SPA_POD_BUILDER_INIT(pod_buffer, sizeof(pod_buffer));
    spa_audio_info_raw audio{};
    audio.format = SPA_AUDIO_FORMAT_F32;
    audio.rate = config.sample_rate;
    audio.channels = config.channels;
    const spa_pod* params[] = {spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &audio)};
    pw_thread_loop_lock(loop_);
    const int connect_result = pw_stream_connect(stream_, PW_DIRECTION_INPUT, PW_ID_ANY,
                          static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
                          params, 1);
    pw_thread_loop_unlock(loop_);
    if (connect_result < 0) { stop(); return false; }
    active_ = true;
    return true;
  }
  bool connected() const noexcept override { return active_; }
  bool process(std::span<const float>, std::uint64_t, bool, double, bool, bool,
               MonitorStream&) noexcept override { return active_; }
  void report_error(int error_code, MonitorStream& handoff) noexcept override {
    active_ = false; handoff.on_error(error_code);
  }
  void stop() noexcept override {
    active_ = false;
    if (handoff_) handoff_->stop();
    if (loop_ && loop_started_) pw_thread_loop_lock(loop_);
    if (stream_) {
      pw_stream_disconnect(stream_);
      pw_stream_destroy(stream_);
      stream_ = nullptr;
    }
    if (loop_ && loop_started_) pw_thread_loop_unlock(loop_);
    // Stop the loop before tearing down objects it may still dispatch against.
    if (loop_ && loop_started_) {
      pw_thread_loop_stop(loop_);
      loop_started_ = false;
    }
    if (props_) { pw_properties_free(props_); props_ = nullptr; }
    if (core_) { pw_core_disconnect(core_); core_ = nullptr; }
    if (context_) { pw_context_destroy(context_); context_ = nullptr; }
    if (loop_) {
      pw_thread_loop_destroy(loop_); loop_ = nullptr; loop_started_ = false;
    }
    handoff_ = nullptr;
  }
private:
  static void destroy(void* data) {
    auto& owner = *static_cast<LinuxStreamOwner*>(data);
    owner.active_ = false;
    if (owner.handoff_) owner.handoff_->on_error(-EPIPE);
  }
  static void param_changed(void* data, uint32_t id, const spa_pod* param) {
    auto& owner = *static_cast<LinuxStreamOwner*>(data);
    if (id == SPA_PARAM_Format) {
      spa_audio_info_raw negotiated{};
      const bool valid = param && spa_format_audio_raw_parse(param, &negotiated) == 0 &&
                         negotiated.format == SPA_AUDIO_FORMAT_F32 &&
                         negotiated.rate == owner.sample_rate_ && negotiated.channels == owner.channels_;
      if (valid) return;
      owner.active_ = false;
      if (owner.handoff_) owner.handoff_->on_error(-EPROTO);
    }
  }
  static void state_changed(void* data, enum pw_stream_state, enum pw_stream_state state,
                            const char* error) {
    auto& owner = *static_cast<LinuxStreamOwner*>(data);
    if (state == PW_STREAM_STATE_ERROR || state == PW_STREAM_STATE_UNCONNECTED ||
        state == PW_STREAM_STATE_PAUSED) {
      owner.active_ = false;
      if (owner.handoff_) owner.handoff_->on_error(error ? -EIO : -EPIPE);
    }
  }
  static void process(void* data) {
    auto& owner = *static_cast<LinuxStreamOwner*>(data);
    if (!owner.active_ || !owner.stream_ || !owner.handoff_) return;
    auto* buffer = pw_stream_dequeue_buffer(owner.stream_);
    if (!buffer) return;
    if (!buffer->buffer || buffer->buffer->n_datas == 0 || !buffer->buffer->datas[0].data) {
      owner.active_ = false;
      if (owner.handoff_) owner.handoff_->on_error(-EINVAL);
      pw_stream_queue_buffer(owner.stream_, buffer);
      return;
    }
    const auto& data_block = buffer->buffer->datas[0];
    if (!data_block.chunk) {
      pw_stream_queue_buffer(owner.stream_, buffer);
      return;
    }
    const auto sample_bytes = data_block.chunk->size;
    if (sample_bytes == 0 || sample_bytes % sizeof(float) != 0 ||
        data_block.chunk->stride != static_cast<int32_t>(sizeof(float))) {
      pw_stream_queue_buffer(owner.stream_, buffer);
      return;
    }
    if (data_block.chunk->offset % sizeof(float) != 0 ||
        data_block.chunk->offset > data_block.maxsize ||
        sample_bytes > data_block.maxsize - data_block.chunk->offset) {
      pw_stream_queue_buffer(owner.stream_, buffer);
      return;
    }
    const auto* sample_data = static_cast<const std::byte*>(data_block.data) + data_block.chunk->offset;
    if (reinterpret_cast<std::uintptr_t>(sample_data) % alignof(float) != 0) {
      pw_stream_queue_buffer(owner.stream_, buffer);
      return;
    }
    const auto samples = std::span<const float>(reinterpret_cast<const float*>(sample_data), sample_bytes / sizeof(float));
    if (!owner.handoff_->push(samples, 0, false, false, 0.0, false)) {
      owner.active_ = false;
      owner.handoff_->on_error(-ENOBUFS);
    }
    pw_stream_queue_buffer(owner.stream_, buffer);
  }
  static constexpr pw_stream_events events_ = {
      .version = PW_VERSION_STREAM_EVENTS, .destroy = destroy, .state_changed = state_changed,
      .param_changed = param_changed, .process = process};
  static const pw_stream_events events;
  pw_thread_loop* loop_{};
  pw_context* context_{};
  pw_core* core_{};
  pw_stream* stream_{};
  pw_properties* props_{};
  MonitorStream* handoff_{};
  bool active_ = false;
  bool loop_started_ = false;
  std::uint32_t sample_rate_{};
  std::uint32_t channels_{};
};

const pw_stream_events LinuxStreamOwner::events = LinuxStreamOwner::events_;

std::unique_ptr<StreamOwner> make_linux_stream_owner() { return std::make_unique<LinuxStreamOwner>(); }
}
#endif
