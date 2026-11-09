#include "beatglass/capture/pipewire_source.hpp"

namespace beatglass::pipewire {
MonitorStream::MonitorStream(std::uint64_t epoch, std::uint32_t sample_rate, std::uint32_t channels) noexcept
    : fifo_(std::make_unique<coreaudio::CaptureFifo>(epoch, sample_rate, channels)) {}
bool MonitorStream::push(std::span<const float> samples, std::uint64_t host_time, bool host_time_valid,
                         bool discontinuity, double source_sample_time, bool sample_time_valid) noexcept {
  return !stopped_ && fifo_->push(samples, host_time, host_time_valid, discontinuity, source_sample_time, sample_time_valid);
}
bool MonitorStream::pop(coreaudio::CaptureBlock& block) noexcept { return fifo_ && fifo_->pop(block); }
void MonitorStream::stop() noexcept { stopped_ = true; }
bool MonitorStream::overflowed() const noexcept { return fifo_ && fifo_->exhausted(); }
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
