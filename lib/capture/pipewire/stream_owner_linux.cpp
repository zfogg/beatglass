// Linux-only PipeWire integration seam. Native construction remains provisional
// until a Linux host validates SPA negotiation and pw_stream callback timing.
#if defined(BEATGLASS_HAS_PIPEWIRE)
#include "beatglass/capture/detail/pipewire_stream_owner.hpp"
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/param.h>

namespace beatglass::pipewire::detail {
// This translation unit intentionally isolates PipeWire headers and the future
// pw_stream callback boundary from the portable StreamOwner contract. The
// callback must dequeue only bounded buffers and hand copied float PCM to
// MonitorStream; it must never allocate, block, or touch UI/database state.
static_assert(PW_VERSION_STREAM_EVENTS > 0, "PipeWire stream callback ABI required");
static_assert(SPA_PARAM_FORMAT > 0, "SPA format negotiation required");

class LinuxStreamOwner final : public StreamOwner {
public:
  ~LinuxStreamOwner() override { stop(); }
  bool connect(const Config& config, MonitorStream& handoff) override {
    if (active_ || config.serial.empty() || !config.sample_rate || !config.channels) return false;
    handoff_ = &handoff;
    loop_ = pw_thread_loop_new("beatglass-pipewire", nullptr);
    if (!loop_) return false;
    context_ = pw_context_new(pw_thread_loop_get_loop(loop_), nullptr, 0);
    if (!context_) { stop(); return false; }
    core_ = pw_context_connect(context_, nullptr, 0);
    if (!core_) { stop(); return false; }
    props_ = pw_properties_new(PW_KEY_TARGET_OBJECT, config.serial.c_str(), nullptr);
    if (!props_) { stop(); return false; }
    stream_ = pw_stream_new_simple(pw_thread_loop_get_loop(loop_), "beatglass-monitor", props_, nullptr, this);
    if (!stream_) { stop(); return false; }
    pw_thread_loop_start(loop_);
    if (pw_stream_connect(stream_, PW_DIRECTION_INPUT, PW_ID_ANY,
                          static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
                          nullptr, 0) < 0) { stop(); return false; }
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
    if (stream_) { pw_stream_destroy(stream_); stream_ = nullptr; }
    if (props_) { pw_properties_free(props_); props_ = nullptr; }
    if (core_) { pw_core_disconnect(core_); core_ = nullptr; }
    if (context_) { pw_context_destroy(context_); context_ = nullptr; }
    if (loop_) { pw_thread_loop_stop(loop_); pw_thread_loop_destroy(loop_); loop_ = nullptr; }
    handoff_ = nullptr;
  }
private:
  pw_thread_loop* loop_{};
  pw_context* context_{};
  pw_core* core_{};
  pw_stream* stream_{};
  pw_properties* props_{};
  MonitorStream* handoff_{};
  bool active_ = false;
};

std::unique_ptr<StreamOwner> make_linux_stream_owner() { return std::make_unique<LinuxStreamOwner>(); }
}
#endif
