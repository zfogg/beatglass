#pragma once
#include "beatglass/capture/source.hpp"
#include <atomic>
#include <cstdint>
#include <limits>
#include <optional>
namespace beatglass {
/** Result of applying a backend notification. */
enum class SourceEventResult { accepted, stale, invalid, sequence_gap };
/** Process-session epoch allocator owned by the source coordinator.
 * Construct exactly one allocator per process session and inject that same
 * object into live and offline control/workers. Keep it alive until every
 * caller has stopped and across source recreation. Calls to next() may be
 * concurrent, but this object may use non-lock-free atomics and must never be
 * called from the audio callback. Unique issuance alone does not synchronize
 * audio, format installation or event delivery. Requests for one source still
 * need serialized lifecycle transitions; a later-issued epoch may complete
 * before an earlier request. A new process session needs a distinct session
 * identity or a proven drain before epoch values can be reused.
 */
class SourceEpochAllocator {
public:
  /** Begin after last_issued, normally zero for a new drained session.
   * A restored nonzero value is safe only if it is at least every issued epoch
   * that can remain in flight for this session. Never construct independent
   * allocators for concurrent streams in the same session.
   */
  explicit SourceEpochAllocator(std::uint64_t last_issued = 0) noexcept : last_(last_issued) {}
  /** Return a unique nonzero epoch, or empty on exhaustion without wrapping.
   * Concurrent calls are linearized at issuance; do not infer completion order.
   */
  std::optional<std::uint64_t> next() noexcept {
    auto current = last_.load(std::memory_order_relaxed);
    while (current != std::numeric_limits<std::uint64_t>::max()) {
      if (last_.compare_exchange_weak(current, current + 1, std::memory_order_relaxed)) return current + 1;
    }
    return std::nullopt;
  }
private:
  std::atomic<std::uint64_t> last_{};
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
  /** Begin an attempt from stopped, interrupted, or error; generation exhaustion fails. */
  std::optional<SourceAttempt> start() noexcept {
    if ((state_ != SourceState::stopped && state_ != SourceState::interrupted && state_ != SourceState::error) || generation_ == std::numeric_limits<std::uint64_t>::max()) return std::nullopt;
    ++generation_; last_sequence_ = 0; epoch_.reset(); state_ = SourceState::starting; return SourceAttempt{generation_};
  }
  /** Apply an ordered transition. Running requires a fresh, nonzero epoch; other states require no epoch.
   * Old attempts and duplicate numbers are stale. A gap or invalid transition
   * invalidates the epoch and enters error; the owner must clear publication.
   * Invalid input never publishes an active epoch.
   */
  SourceEventResult notify(SourceAttempt attempt, std::uint64_t sequence, SourceState next, std::optional<std::uint64_t> epoch = std::nullopt) noexcept {
    if (state_ == SourceState::stopped || state_ == SourceState::error || attempt.generation != generation_ || generation_ == 0 || sequence <= last_sequence_) return SourceEventResult::stale;
    if (last_sequence_ == std::numeric_limits<std::uint64_t>::max() || sequence != last_sequence_ + 1) { fail_closed(); return SourceEventResult::sequence_gap; }
    last_sequence_ = sequence;
    if (next == SourceState::running && epoch && *epoch && (state_ == SourceState::starting || state_ == SourceState::interrupted) && (!last_epoch_ || *epoch > *last_epoch_)) { epoch_ = epoch; last_epoch_ = epoch; state_ = next; return SourceEventResult::accepted; }
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
