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
/** Raw PipeWire graph clock snapshot; it is not a sample-position anchor. */
struct MonitorGraphClock {
  std::int64_t now_ns{}; ///< Driver clock time of this snapshot.
  std::uint64_t ticks{}; ///< Driver graph position at now_ns.
  std::uint32_t rate_num{}, rate_denom{}; ///< Seconds per graph tick.
  std::int64_t delay{}; ///< Backend-reported graph delay in graph ticks.
  std::uint64_t queued{}; ///< PipeWire queued size in the backend's buffer units.
};
/** Ordered event for one start attempt; a loss indication invalidates all evidence. */
struct MonitorEvent {
  SourceAttempt attempt{}; ///< Attempt allocated by the control owner.
  std::uint64_t sequence{}; ///< Consecutive event number, or zero for delivery loss.
  std::optional<std::uint64_t> epoch; ///< Present only for a running event.
  SourceState state{SourceState::stopped}; ///< Source state after the event.
  std::optional<AudioClockAnchor> anchor; ///< Absent until an anchor is measured.
  std::optional<SourceError> error; ///< Typed diagnostic for an error, when known.
  bool delivery_lost{}; ///< True invalidates the attempt and discards queued events.
};
/** Enumerate sink identities using the shared typed result; no cancellation is supported yet. */
EnumerationResult enumerate_monitor_sources(std::chrono::milliseconds timeout);
/**
 * Private PipeWire sink-monitor prototype. The control thread owns start, stop,
 * state and lifetime; one worker may pop. The worker must be stopped before
 * stop/destruction. One instance supports one attempt and one supplied epoch.
 * This is not an AudioSource implementation or an installed API. Running is
 * gated on a registry sync and both monitor-port links from the selected sink.
 * Events are ordered and bounded; the worker must explicitly install the
 * epoch/format. Ordering between graph changes and queued audio remains unresolved.
 */
class Monitor {
public:
  /** Create an instance with a process-unique nonzero epoch and nonzero attempt. */
  explicit Monitor(std::uint64_t epoch, SourceAttempt attempt);
  /** Destroy after quiescing the consumer; also stops the PipeWire loop. */
  ~Monitor();
  Monitor(const Monitor&) = delete;
  Monitor& operator=(const Monitor&) = delete;
  /** Validate the selected serial/name against discovery, then request capture; acceptance is not running. Removal after validation remains a race. */
  SourceStartResult start(const Sink& sink);
  /** Stop on control thread and join the callback loop; no restart is supported. */
  void stop() noexcept;
  /** Read an atomic status; any non-streaming state invalidates retained analysis. */
  MonitorState state() const noexcept;
  /** Pop the next ordered event on the sole control consumer. Delivery loss is
   * returned out of band and invalidates the entire attempt; queued events are discarded. */
  bool pop_event(MonitorEvent& event) noexcept;
  /** Install the running epoch and format on the sole worker after receiving and applying the ordered running event and resetting analysis. Reject stale or mismatched values. This does not order graph changes against already published results. */
  bool install_worker_epoch(std::uint64_t epoch, AudioFormat format) noexcept;
  /** Copy on the sole consumer after installation; caller must reject old data and recheck status. */
  bool pop(MonitorIngress::Block& block) noexcept;
  /** Read a coherent measured graph-clock snapshot, if available. It cannot be
   * mapped to a captured sample until buffering and clock domain are established. */
  std::optional<MonitorGraphClock> graph_clock() const noexcept;
  /** Backend diagnostic code, provisional and zero when no error has been recorded. */
  int diagnostic_code() const noexcept;
  /** Count complete frames dropped because bounded storage was full or too small. */
  std::uint64_t dropped_frames() const noexcept;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
