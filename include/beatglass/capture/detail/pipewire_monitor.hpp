#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include "beatglass/capture/detail/pipewire_ingress.hpp"
#include "beatglass/capture/source.hpp"
#include "beatglass/core/audio_clock.hpp"
#include "beatglass/capture/detail/pipewire_discovery.hpp"
namespace beatglass::pipewire {
/** Provisional monitor state; interruption and error require a new instance. */
enum class MonitorState : unsigned { stopped, starting, streaming, interrupted, error };
/** Ordered event for one start attempt; a loss indication invalidates all evidence. */
struct MonitorEvent {
  SourceAttempt attempt{};
  std::uint64_t sequence{};
  std::optional<std::uint64_t> epoch;
  SourceState state{SourceState::stopped};
  std::optional<AudioClockAnchor> anchor;
  bool delivery_lost{};
};
/**
 * Private PipeWire sink-monitor prototype. The control thread owns start, stop,
 * state and lifetime; one worker may pop. The worker must be stopped before
 * stop/destruction. One instance supports one attempt and one supplied epoch.
 * This is not an AudioSource implementation or an installed API. Running is
 * gated on a registry sync and both monitor-port links from the selected sink.
 * Events are ordered and bounded, but a control-to-worker epoch/format barrier
 * and ordering between graph changes and already queued audio remain unresolved.
 */
class Monitor {
public:
  /** Create an instance with a process-unique nonzero epoch and nonzero attempt. */
  explicit Monitor(std::uint64_t epoch, SourceAttempt attempt);
  /** Destroy after quiescing the consumer; also stops the PipeWire loop. */
  ~Monitor();
  Monitor(const Monitor&) = delete;
  Monitor& operator=(const Monitor&) = delete;
  /** Validate the selected serial/name against discovery, then request capture; true is not running. Removal after validation remains a race. */
  bool start(const Sink& sink);
  /** Stop on control thread and join the callback loop; no restart is supported. */
  void stop() noexcept;
  /** Read an atomic status; any non-streaming state invalidates retained analysis. */
  MonitorState state() const noexcept;
  /** Pop the next ordered event on the sole control consumer. Delivery loss is
   * returned out of band and invalidates the entire attempt; queued events are discarded. */
  bool pop_event(MonitorEvent& event) noexcept;
  /** Copy on the sole consumer; caller must install epoch/format, reject old data and recheck status. */
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
