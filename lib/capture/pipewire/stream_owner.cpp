#include "beatglass/capture/detail/pipewire_stream_owner.hpp"
#include <cerrno>
namespace beatglass::pipewire::detail {
namespace {
class UnavailableStreamOwner final : public StreamOwner {
public:
  bool connect(const std::string&, std::uint32_t, std::uint32_t,
               MonitorStream& handoff) override {
    handoff.on_error(-ENOSYS);
    return false;
  }
  void stop() noexcept override {}
};
}
std::unique_ptr<StreamOwner> make_stream_owner() {
  return std::make_unique<UnavailableStreamOwner>();
}
}
