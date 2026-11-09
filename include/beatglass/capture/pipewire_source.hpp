#pragma once

#include "beatglass/capture/detail/pipewire_discovery.hpp"
#include "beatglass/capture/coreaudio_fifo_internal.hpp"
#include <chrono>
#include <cstdint>
#include <memory>
#include <span>

namespace beatglass::pipewire {
/** Control-thread lifecycle for a future PipeWire output-monitor stream.
 * Implementations own the PipeWire stream and never expose callback memory;
 * capture callbacks copy bounded PCM into caller-owned storage. A source
 * switch, disconnect, format change, or overflow must publish discontinuity
 * and require a fresh nonzero epoch before data is accepted.
 */
class MonitorBackend {
public:
  virtual ~MonitorBackend() = default;
  /** Enumerate output sinks with a bounded control-thread timeout. */
  virtual Discovery enumerate(std::chrono::milliseconds timeout) = 0;
  /** Select a sink and begin a fresh epoch; false leaves status observable. */
  virtual bool start(const std::string& serial, std::uint64_t epoch) = 0;
  /** Stop and quiesce the stream; safe to call repeatedly. */
  virtual bool stop() noexcept = 0;
};
/** Bounded callback handoff used by a PipeWire stream adapter. The callback only calls push; control code calls pop/stop. */
class MonitorStream {
public:
  MonitorStream(std::uint64_t epoch, std::uint32_t sample_rate, std::uint32_t channels) noexcept;
  /** Copy one callback block into preallocated storage; false records overflow/discontinuity. */
  bool push(std::span<const float> samples, std::uint64_t host_time, bool host_time_valid,
            bool discontinuity, double source_sample_time, bool sample_time_valid) noexcept;
  /** Pop the next bounded block on the control thread. */
  bool pop(coreaudio::CaptureBlock& block) noexcept;
  /** Stop accepting callback data and expose overflow state. */
  void stop() noexcept;
  bool overflowed() const noexcept;
private:
  std::unique_ptr<coreaudio::CaptureFifo> fifo_;
  bool stopped_{};
};
/** Create the provisional PipeWire monitor boundary; ownership stays with the caller. */
std::unique_ptr<MonitorBackend> make_monitor_backend();
}
