#pragma once
#include "beatglass/core/results.hpp"
#include <cstdint>
#include <limits>
namespace beatglass {
/// Identifies one acquisition attempt and its source epoch.
struct AcquisitionToken {
  std::uint64_t epoch{}; ///< Epoch whose samples feed the attempt.
  std::uint64_t generation{}; ///< Monotonically increasing attempt identifier.
};
/**
 * Worker-owned musical tracking lifecycle; not thread-safe.
 *
 * This class does not classify silence or estimate music: its caller supplies
 * those observations. Tokens prevent results from a prior attempt from
 * reviving state after silence, discontinuity, expiry, stop or failure. The
 * caller must separately clear displayed estimates whenever a transition
 * invalidates evidence. No method allocates, blocks or invokes callbacks.
 */
class TrackingMachine {
public:
  /// Current lifecycle state. Initially stopped.
  TrackingState state() const noexcept { return state_; }
  /// Current acquisition token. Valid for results only while acquiring,
  /// reacquiring or tracking and only if no later transition invalidated it.
  AcquisitionToken token() const noexcept { return {epoch_, generation_}; }
  /** Start or restart after stop/error on the worker thread.
   * @param epoch Source epoch of the new session.
   * @return False if already active or generation space is exhausted.
   */
  bool start(std::uint64_t epoch) noexcept {
    if (state_ != TrackingState::stopped && state_ != TrackingState::error) return false;
    if (!advance()) return false;
    epoch_ = epoch; state_ = TrackingState::acquiring; return true;
  }
  /// End the session; pending results are rejected until a new start.
  void stop() noexcept { state_ = TrackingState::stopped; }
  /// Record a source or estimator error; pending results are rejected.
  void fail() noexcept { if (state_ != TrackingState::stopped) state_ = TrackingState::error; }
  /** Invalidate evidence on a gap, reset, format change or source interruption.
   * @param epoch Epoch of the next accepted samples.
   * @return False if inactive or generation space is exhausted.
   */
  bool discontinuity(std::uint64_t epoch) noexcept {
    if (!active() || !advance()) return false;
    epoch_ = epoch; state_ = TrackingState::reacquiring; return true;
  }
  /// Mark a period of silence and invalidate prior estimates once per period.
  bool silence() noexcept {
    if (!active()) return false;
    if (state_ == TrackingState::silent) return true;
    if (!advance()) return false;
    state_ = TrackingState::silent; return true;
  }
  /// Signal resumption after silence; other active states remain unchanged.
  bool signal() noexcept {
    if (!active()) return false;
    if (state_ == TrackingState::silent) state_ = TrackingState::acquiring;
    return true;
  }
  /// Expire stale evidence, requiring a fresh acquisition attempt.
  bool expire() noexcept {
    if (!active() || state_ == TrackingState::silent || !advance()) return false;
    state_ = TrackingState::reacquiring; return true;
  }
  /** Accept evidence from the current attempt if it is still current.
   * @param result_token Epoch and generation captured when work was started.
   * @return True if accepted; false for stale tokens or inactive/silent state.
   */
  bool accept(AcquisitionToken result_token) noexcept {
    if (!active() || state_ == TrackingState::silent || result_token.epoch != epoch_ || result_token.generation != generation_) return false;
    state_ = TrackingState::tracking; return true;
  }
private:
  bool active() const noexcept { return state_ != TrackingState::stopped && state_ != TrackingState::error; }
  bool advance() noexcept {
    if (generation_ == std::numeric_limits<std::uint64_t>::max()) { state_ = TrackingState::error; return false; }
    ++generation_; return true;
  }
  TrackingState state_{TrackingState::stopped};
  std::uint64_t epoch_{}, generation_{};
};
}
