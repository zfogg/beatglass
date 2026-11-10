#if defined(BEATGLASS_HAS_PIPEWIRE)

#include "beatglass/desktop/pipewire_playback_session.hpp"
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/param.h>
#include <spa/buffer/buffer.h>
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <mutex>

namespace beatglass::pipewire {
class PlaybackSession::Impl {
public:
  Impl() : queue(8, 4096) {}
  PlaybackPcmQueue queue;
  pw_thread_loop* loop{};
  pw_context* context{};
  pw_core* core{};
  pw_stream* stream{};
  pw_properties* props{};
  std::atomic<bool> active{false};
  std::atomic<int> error{0};
  std::mutex status_mutex;
  std::string status{"Playback stopped."};
  std::uint32_t sample_rate{};
  std::uint16_t channels{};
  bool loop_started{};

  static void destroy(void* data) {
    auto& self = *static_cast<Impl*>(data);
    self.active.store(false, std::memory_order_release);
    self.error.store(-EPIPE, std::memory_order_release);
  }
  static void state_changed(void* data, enum pw_stream_state, enum pw_stream_state state,
                            const char* error) {
    auto& self = *static_cast<Impl*>(data);
    if (state == PW_STREAM_STATE_ERROR || state == PW_STREAM_STATE_UNCONNECTED) {
      self.active.store(false, std::memory_order_release);
      self.error.store(error ? -EIO : -EPIPE, std::memory_order_release);
    }
  }
  static void param_changed(void* data, uint32_t id, const spa_pod* param) {
    if (id != SPA_PARAM_Format || !param) return;
    auto& self = *static_cast<Impl*>(data);
    spa_audio_info_raw negotiated{};
    if (spa_format_audio_raw_parse(param, &negotiated) != 0 ||
        negotiated.format != SPA_AUDIO_FORMAT_F32 || negotiated.rate != self.sample_rate ||
        negotiated.channels != self.channels) {
      self.active.store(false, std::memory_order_release);
      self.error.store(-EPROTO, std::memory_order_release);
    }
  }
  static void process(void* data) {
    auto& self = *static_cast<Impl*>(data);
    if (!self.active.load(std::memory_order_acquire) || !self.stream) return;
    auto* buffer = pw_stream_dequeue_buffer(self.stream);
    if (!buffer) return;
    auto* spa_buffer = buffer->buffer;
    if (!spa_buffer || spa_buffer->n_datas == 0 || !spa_buffer->datas[0].data ||
        !spa_buffer->datas[0].chunk) {
      pw_stream_queue_buffer(self.stream, buffer);
      self.error.store(-EINVAL, std::memory_order_release);
      self.active.store(false, std::memory_order_release);
      return;
    }
    auto& data_block = spa_buffer->datas[0];
    const auto offset = data_block.chunk->offset;
    const auto maxsize = data_block.maxsize;
    if (offset > maxsize || maxsize - offset < sizeof(float)) {
      pw_stream_queue_buffer(self.stream, buffer);
      self.error.store(-EINVAL, std::memory_order_release);
      self.active.store(false, std::memory_order_release);
      return;
    }
    const auto capacity = (maxsize - offset) / sizeof(float);
    auto* output = static_cast<float*>(data_block.data) + offset / sizeof(float);
    AudioBlock block{};
    const auto written = self.queue.pop(std::span<float>(output, capacity), block);
    if (written) {
      data_block.chunk->offset = static_cast<std::uint32_t>(offset);
      data_block.chunk->size = static_cast<std::uint32_t>(block.samples.size() * sizeof(float));
      data_block.chunk->stride = static_cast<std::int32_t>(sizeof(float));
    } else {
      std::fill_n(output, capacity, 0.0f);
      data_block.chunk->offset = static_cast<std::uint32_t>(offset);
      data_block.chunk->size = static_cast<std::uint32_t>(capacity * sizeof(float));
      data_block.chunk->stride = static_cast<std::int32_t>(sizeof(float));
    }
    pw_stream_queue_buffer(self.stream, buffer);
  }
  static constexpr pw_stream_events events{
      .version = PW_VERSION_STREAM_EVENTS, .destroy = destroy, .state_changed = state_changed,
      .param_changed = param_changed, .process = process};
};

constexpr pw_stream_events PlaybackSession::Impl::events;

PlaybackSession::PlaybackSession() : impl_(std::make_unique<Impl>()) {}
PlaybackSession::~PlaybackSession() { stop(); }

bool PlaybackSession::start(std::uint32_t rate, std::uint16_t channels, std::size_t max_samples,
                            std::uint64_t epoch, std::string& error) {
  if (!rate || !channels || !max_samples || max_samples > 4096 || !epoch ||
      !session_.start(epoch)) {
    error = "invalid PipeWire playback session dimensions or epoch";
    return false;
  }
  impl_->sample_rate = rate;
  impl_->channels = channels;
  impl_->loop = pw_thread_loop_new("beatglass-pipewire-playback", nullptr);
  if (!impl_->loop) { error = "PipeWire thread loop could not be created"; session_.stop(); return false; }
  impl_->context = pw_context_new(pw_thread_loop_get_loop(impl_->loop), nullptr, 0);
  if (!impl_->context) { error = "PipeWire context could not be created"; stop(); return false; }
  impl_->core = pw_context_connect(impl_->context, nullptr, 0);
  if (!impl_->core) { error = "PipeWire daemon connection failed"; stop(); return false; }
  impl_->props = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Playback", nullptr);
  impl_->stream = pw_stream_new_simple(pw_thread_loop_get_loop(impl_->loop), "beatglass-playback",
                                       impl_->props, &Impl::events, impl_.get());
  if (!impl_->stream) { error = "PipeWire playback stream could not be created"; stop(); return false; }
  if (pw_thread_loop_start(impl_->loop) < 0) { error = "PipeWire playback loop could not start"; stop(); return false; }
  impl_->loop_started = true;
  std::uint8_t pod_buffer[1024];
  spa_pod_builder builder = SPA_POD_BUILDER_INIT(pod_buffer, sizeof(pod_buffer));
  spa_audio_info_raw audio{}; audio.format = SPA_AUDIO_FORMAT_F32; audio.rate = rate; audio.channels = channels;
  const spa_pod* params[] = {spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &audio)};
  pw_thread_loop_lock(impl_->loop);
  const auto result = pw_stream_connect(impl_->stream, PW_DIRECTION_OUTPUT, PW_ID_ANY,
      static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS), params, 1);
  pw_thread_loop_unlock(impl_->loop);
  if (result < 0) { error = "PipeWire playback stream negotiation failed"; stop(); return false; }
  impl_->active.store(true, std::memory_order_release);
  { std::lock_guard lock(impl_->status_mutex); impl_->status = "Playing decoded PCM through PipeWire output."; }
  return true;
}

bool PlaybackSession::submit(const AudioBlock& block) noexcept {
  return session_.state() == PlaybackState::playing && block.epoch == session_.epoch() && impl_->queue.push(block);
}
bool PlaybackSession::active() const noexcept { return impl_->active.load(std::memory_order_acquire); }
bool PlaybackSession::pause() noexcept {
  if (!session_.pause()) return false;
  if (!impl_->loop || !impl_->loop_started || !impl_->stream) {
    session_.resume();
    return false;
  }
  pw_thread_loop_lock(impl_->loop);
  const auto result = pw_stream_set_active(impl_->stream, false);
  pw_thread_loop_unlock(impl_->loop);
  if (result < 0) {
    session_.resume();
    return false;
  }
  { std::lock_guard lock(impl_->status_mutex); impl_->status = "Playback paused."; }
  return true;
}
bool PlaybackSession::resume() noexcept {
  if (!session_.resume()) return false;
  if (!impl_->loop || !impl_->loop_started || !impl_->stream) {
    session_.pause();
    return false;
  }
  pw_thread_loop_lock(impl_->loop);
  const auto result = pw_stream_set_active(impl_->stream, true);
  pw_thread_loop_unlock(impl_->loop);
  if (result < 0) {
    session_.pause();
    return false;
  }
  { std::lock_guard lock(impl_->status_mutex); impl_->status = "Playback resumed."; }
  return true;
}
void PlaybackSession::stop() noexcept {
  session_.stop();
  impl_->active.store(false, std::memory_order_release);
  if (impl_->loop && impl_->loop_started) pw_thread_loop_lock(impl_->loop);
  if (impl_->stream) { pw_stream_disconnect(impl_->stream); pw_stream_destroy(impl_->stream); impl_->stream = nullptr; }
  if (impl_->loop && impl_->loop_started) pw_thread_loop_unlock(impl_->loop);
  if (impl_->loop && impl_->loop_started) { pw_thread_loop_stop(impl_->loop); impl_->loop_started = false; }
  if (impl_->props) { pw_properties_free(impl_->props); impl_->props = nullptr; }
  if (impl_->core) { pw_core_disconnect(impl_->core); impl_->core = nullptr; }
  if (impl_->context) { pw_context_destroy(impl_->context); impl_->context = nullptr; }
  if (impl_->loop) { pw_thread_loop_destroy(impl_->loop); impl_->loop = nullptr; }
  { std::lock_guard lock(impl_->status_mutex); impl_->status = "Playback stopped."; }
}
std::string PlaybackSession::status() const { std::lock_guard lock(impl_->status_mutex); return impl_->status; }
}
#endif
