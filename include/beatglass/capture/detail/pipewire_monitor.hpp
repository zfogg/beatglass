#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include "beatglass/capture/detail/pipewire_ingress.hpp"
namespace beatglass::pipewire {
/** Provisional monitor state; interruption and error require a new instance. */
enum class MonitorState : unsigned { stopped, starting, streaming, interrupted, error };
/**
 * Private PipeWire sink-monitor prototype. The control thread owns start, stop,
 * state and lifetime; one worker may pop. The worker must be stopped before
 * stop/destruction. One instance supports one attempt and one supplied epoch.
 * This is not an AudioSource implementation or an installed API. Status is a
 * polling hint, not an ordered notification; production integration must add
 * fail-closed ordered event delivery and an epoch/format installation barrier.
 */
class Monitor {
public:
  /** Create an instance with a process-unique nonzero epoch. */
  explicit Monitor(std::uint64_t epoch);
  /** Destroy after quiescing the consumer; also stops the PipeWire loop. */
  ~Monitor();
  Monitor(const Monitor&) = delete;
  Monitor& operator=(const Monitor&) = delete;
  /** Request capture of the selected server-session sink serial; true is not running. */
  bool start(const std::string& serial);
  /** Stop on control thread and join the callback loop; no restart is supported. */
  void stop() noexcept;
  /** Read an atomic status; any non-streaming state invalidates retained analysis. */
  MonitorState state() const noexcept;
  /** Copy one block on the sole consumer; caller must recheck state and epoch. */
  bool pop(MonitorIngress::Block& block) noexcept;
  /** Backend diagnostic code, provisional and zero when no error has been recorded. */
  int diagnostic_code() const noexcept;
  /** Count complete frames dropped because bounded storage was full or too small. */
  std::uint64_t dropped_frames() const noexcept;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
