#pragma once
#include "beatglass/capture/source.hpp"
#include <cstdint>
#include <limits>
#include <optional>
namespace beatglass {
/** Result of applying a backend notification. */
enum class SourceEventResult { accepted, stale, invalid, sequence_gap };
/** Process-session epoch allocator owned by the serialized source coordinator.
 * Keep this object across source recreation and do not use it concurrently.
 * A new process session needs a distinct session identity or a proven drain.
 */
class SourceEpochAllocator {
public:
  /** Return the next nonzero epoch, or empty on exhaustion without wrapping. */
  std::optional<std::uint64_t> next() noexcept {
    if (last_ == std::numeric_limits<std::uint64_t>::max()) return std::nullopt;
    return ++last_;
  }
private:
  std::uint64_t last_{};
};
/** Serial control-thread lifecycle; not thread-safe or callable from audio callbacks.
 * An adapter-owned serial dispatcher must establish causal order before assigning
 * consecutive numbers starting at one for each attempt. A delivery overflow or
 * sequence gap fails closed: the owner must invalidate evidence, quiesce and
 * restart as appropriate. Retain this helper across source recreation or drain
 * all old notifications before replacing it. It does not perform those operations. Epochs
 * are allocated by the process coordinator across object recreation and never
 * reused while old publications can remain in flight. Session identity or a
 * proven drain is required across process restarts.
 */
class SourceLifecycle {
public:
  /** Current control-thread state. */
  SourceState state() const noexcept { return state_; }
  /** Current epoch, absent unless running. */
  std::optional<std::uint64_t> epoch() const noexcept { return epoch_; }
  /** Begin an attempt from stopped or error; generation exhaustion fails. */
  std::optional<SourceAttempt> start() noexcept {
    if ((state_ != SourceState::stopped && state_ != SourceState::error) || generation_ == std::numeric_limits<std::uint64_t>::max()) return std::nullopt;
    ++generation_; last_sequence_ = 0; epoch_.reset(); state_ = SourceState::starting; return SourceAttempt{generation_};
  }
  /** Apply an ordered transition. Running requires a fresh epoch; other states require no epoch.
   * Old attempts and duplicate numbers are stale. A gap or invalid transition
   * invalidates the epoch and enters error; the owner must clear publication.
   */
  SourceEventResult notify(SourceAttempt attempt, std::uint64_t sequence, SourceState next, std::optional<std::uint64_t> epoch = std::nullopt) noexcept {
    if (state_ == SourceState::stopped || state_ == SourceState::error || attempt.generation != generation_ || generation_ == 0 || sequence <= last_sequence_) return SourceEventResult::stale;
    if (last_sequence_ == std::numeric_limits<std::uint64_t>::max() || sequence != last_sequence_ + 1) { fail_closed(); return SourceEventResult::sequence_gap; }
    last_sequence_ = sequence;
    if (next == SourceState::running && epoch && (state_ == SourceState::starting || state_ == SourceState::interrupted) && (!last_epoch_ || *epoch > *last_epoch_)) { epoch_ = epoch; last_epoch_ = epoch; state_ = next; return SourceEventResult::accepted; }
    if (!epoch && ((next == SourceState::interrupted && state_ == SourceState::running) || next == SourceState::error)) { epoch_.reset(); state_ = next; return SourceEventResult::accepted; }
    fail_closed(); return SourceEventResult::invalid;
  }
  /** Fail closed on a known control-delivery overflow. */
  void delivery_lost() noexcept { if (state_ != SourceState::stopped) fail_closed(); }
  /** Invalidate the attempt and epoch; caller separately quiesces callbacks. */
  void stop() noexcept { epoch_.reset(); state_ = SourceState::stopped; }
private:
  void fail_closed() noexcept { epoch_.reset(); state_ = SourceState::error; }
  SourceState state_{SourceState::stopped};
  std::uint64_t generation_{}, last_sequence_{};
  std::optional<std::uint64_t> epoch_, last_epoch_;
};
}
