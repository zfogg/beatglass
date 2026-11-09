#include "beatglass/capture/detail/pipewire_stream_owner.hpp"
namespace beatglass::pipewire::detail {
namespace {
class UnavailableStreamOwner final : public StreamOwner {
public:
  bool connect(const std::string&, std::uint32_t, std::uint32_t,
               MonitorStream&) override { return false; }
  void stop() noexcept override {}
};
}
std::unique_ptr<StreamOwner> make_stream_owner() {
  return std::make_unique<UnavailableStreamOwner>();
}
}
