#include "beatglass/capture/detail/pipewire_stream_owner.hpp"
#include <cerrno>
namespace beatglass::pipewire::detail {
namespace {
class UnavailableStreamOwner final : public StreamOwner {
public:
  bool connect(const Config&, MonitorStream& handoff) override {
    handoff.on_error(-ENOSYS);
    return false;
  }
  bool connected() const noexcept override { return false; }
  bool process(std::span<const float>, std::uint64_t, bool, double, bool, bool,
               MonitorStream& handoff) noexcept override {
    handoff.on_error(-ENOSYS);
    return false;
  }
  void report_error(int error_code, MonitorStream& handoff) noexcept override {
    handoff.on_error(error_code);
  }
  void stop() noexcept override {}
};
}
std::unique_ptr<StreamOwner> make_stream_owner() {
  return std::make_unique<UnavailableStreamOwner>();
}
}
