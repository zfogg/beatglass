#include "beatglass/capture/pipewire_source.hpp"

namespace beatglass::pipewire {
namespace {
class DiscoveryBoundary final : public MonitorBackend {
public:
  Discovery enumerate(std::chrono::milliseconds timeout) override { return discover_sinks(timeout); }
  bool start(const std::string&, std::uint64_t) override { return false; }
  bool stop() noexcept override { return true; }
};
}
std::unique_ptr<MonitorBackend> make_monitor_backend() { return std::make_unique<DiscoveryBoundary>(); }
}
