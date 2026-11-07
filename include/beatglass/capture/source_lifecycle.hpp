#pragma once
#include "beatglass/capture/source.hpp"
#include <cstdint>
#include <limits>
#include <optional>

namespace beatglass {
/** Identifies one source start attempt, distinct from its audio epoch. */
struct SourceAttempt {
  /** Unique generation within the lifetime of a lifecycle object. */
  std::uint64_t generation{};
};

/**
 * Control-thread state for an asynchronous source adapter; not thread-safe.
 *
 * Backend notifications must be transferred to the control thread before
 * using this object. The audio callback must not call it. A new attempt token
 * is required on every start; late notifications from previous attempts are
 * rejected. Interruption invalidates the current audio epoch and resumption
 * must use a strictly increasing epoch, including across starts. This class does not start hardware, quiesce
 * callbacks, allocate epochs, or reset estimators; the adapter and consumer
 * remain responsible for those operations.
 */
class SourceLifecycle {
public:
  /** Current control-thread state; initially stopped. */
  SourceState state() const noexcept { return state_; }
  /** Current running epoch, absent while stopped, starting, interrupted or failed. */
  std::optional<std::uint64_t> epoch() const noexcept { return epoch_; }
  /** Begin a new attempt from stopped or error; failure leaves state unchanged. */
  std::optional<SourceAttempt> start() noexcept {
    if ((state_ != SourceState::stopped && state_ != SourceState::error) ||
        generation_ == std::numeric_limits<std::uint64_t>::max()) return std::nullopt;
    ++generation_; epoch_.reset(); state_ = SourceState::starting;
    return SourceAttempt{generation_};
  }
  /** Accept a strictly newer epoch on start or resumption. */
  bool running(SourceAttempt attempt, std::uint64_t epoch) noexcept {
    if (!current(attempt) || (state_ != SourceState::starting && state_ != SourceState::interrupted) ||
        (last_epoch_ && epoch <= *last_epoch_)) return false;
    epoch_ = epoch; last_epoch_ = epoch; state_ = SourceState::running; return true;
  }
  /** Mark a running source interrupted; the prior epoch can no longer be used. */
  bool interrupt(SourceAttempt attempt) noexcept {
    if (!current(attempt) || state_ != SourceState::running) return false;
    epoch_.reset(); state_ = SourceState::interrupted; return true;
  }
  /** Mark the current attempt failed and invalidate its running epoch. */
  bool fail(SourceAttempt attempt) noexcept {
    if (!current(attempt) || state_ == SourceState::stopped || state_ == SourceState::error) return false;
    epoch_.reset(); state_ = SourceState::error; return true;
  }
  /** Invalidate notifications and epoch. The adapter must separately quiesce callbacks. */
  void stop() noexcept { epoch_.reset(); state_ = SourceState::stopped; }
private:
  bool current(SourceAttempt attempt) const noexcept { return generation_ != 0 && generation_ == attempt.generation; }
  SourceState state_{SourceState::stopped};
  std::uint64_t generation_{};
  std::optional<std::uint64_t> epoch_, last_epoch_;
};
}
