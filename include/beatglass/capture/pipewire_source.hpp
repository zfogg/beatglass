#pragma once

#include "beatglass/capture/detail/pipewire_discovery.hpp"
#include <chrono>
#include <cstdint>
#include <memory>

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
/** Create the provisional PipeWire monitor boundary; ownership stays with the caller. */
std::unique_ptr<MonitorBackend> make_monitor_backend();
}
