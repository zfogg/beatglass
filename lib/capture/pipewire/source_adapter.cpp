#include "beatglass/capture/pipewire_source.hpp"

namespace beatglass::pipewire {
MonitorStream::MonitorStream(std::uint64_t epoch, std::uint32_t sample_rate, std::uint32_t channels) noexcept
    : fifo_(std::make_unique<coreaudio::CaptureFifo>(epoch, sample_rate, channels)) {}
bool MonitorStream::push(std::span<const float> samples, std::uint64_t host_time, bool host_time_valid,
                         bool discontinuity, double source_sample_time, bool sample_time_valid) noexcept {
  const bool accepted = !stopped_ && fifo_->push(samples, host_time, host_time_valid,
      discontinuity || discontinuity_pending_.exchange(false, std::memory_order_acq_rel),
      source_sample_time, sample_time_valid);
  return accepted;
}
bool MonitorStream::on_process(std::span<const float> samples, std::uint64_t host_time, bool host_time_valid,
                               double source_sample_time, bool sample_time_valid) noexcept {
  return push(samples, host_time, host_time_valid, false, source_sample_time, sample_time_valid);
}
void MonitorStream::on_error(int error_code) noexcept {
  error_code_.store(error_code, std::memory_order_release);
  discontinuity_pending_.store(true, std::memory_order_release);
  stopped_ = true;
}
int MonitorStream::error_code() const noexcept { return error_code_.load(std::memory_order_acquire); }
int MonitorStream::take_error() noexcept { return error_code_.exchange(0, std::memory_order_acq_rel); }
bool MonitorStream::take_discontinuity() noexcept {
  return discontinuity_pending_.exchange(false, std::memory_order_acq_rel);
}
bool MonitorStream::needs_restart() const noexcept {
  return stopped_.load(std::memory_order_acquire) || error_code() != 0;
}
bool MonitorStream::restart(std::uint64_t epoch, std::uint32_t sample_rate, std::uint32_t channels) {
  if (!epoch || !sample_rate || !channels) return false;
  auto next = std::make_unique<coreaudio::CaptureFifo>(epoch, sample_rate, channels);
  fifo_ = std::move(next);
  error_code_.store(0, std::memory_order_release);
  discontinuity_pending_.store(true, std::memory_order_release);
  stopped_.store(false, std::memory_order_release);
  return true;
}
bool MonitorStream::pop(coreaudio::CaptureBlock& block) noexcept { return fifo_ && fifo_->pop(block); }
void MonitorStream::stop() noexcept {
  discontinuity_pending_.store(true, std::memory_order_release);
  stopped_.store(true, std::memory_order_release);
}
bool MonitorStream::overflowed() const noexcept { return fifo_ && fifo_->exhausted(); }
bool MonitorSession::start(std::string serial, std::uint64_t epoch, std::uint32_t sample_rate,
                           std::uint32_t channels) {
  if (serial.empty() || !epoch || !sample_rate || !channels) return false;
  auto next = std::make_unique<MonitorStream>(epoch, sample_rate, channels);
  serial_ = std::move(serial);
  stream_ = std::move(next);
  return true;
}
void MonitorSession::stop() noexcept { if (stream_) stream_->stop(); }
bool MonitorSession::needs_restart() const noexcept { return !stream_ || stream_->needs_restart(); }
bool MonitorSession::restart(std::uint64_t epoch, std::uint32_t sample_rate, std::uint32_t channels) {
  return stream_ && stream_->restart(epoch, sample_rate, channels);
}
bool MonitorSession::on_process(std::span<const float> samples, std::uint64_t host_time,
                                bool host_time_valid, double source_sample_time,
                                bool sample_time_valid) noexcept {
  return stream_ && stream_->on_process(samples, host_time, host_time_valid,
                                        source_sample_time, sample_time_valid);
}
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
