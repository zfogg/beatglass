#pragma once
#include "beatglass/library/jobs.hpp"
#include <cstdint>
#include <optional>
namespace beatglass {
/** A worker-owned job state and frame progress with an optionally known total. */
struct JobSnapshot {
  /** Current state; terminal states cannot be changed. */
  JobStatus status{JobStatus::queued};
  /** Number of complete input frames processed. */
  std::uint64_t frames_processed{};
  /** Expected input frames, or absent if the total is unknown. */
  std::optional<std::uint64_t> total_frames;
};
/**
 * Serial job lifecycle for one worker; not thread-safe and not a scheduler.
 *
 * Cancellation requests must be delivered to the worker before cancel() is
 * called. The worker arbitrates completion versus cancellation in order: a
 * terminal transition wins and later operations fail. In a persistence job,
 * the owner may call complete only after the storage transaction commits;
 * this object itself neither commits nor rolls back storage. Progress counts
 * complete frames and never regresses or exceeds a known total.
 */
class JobLifecycle {
public:
  /** Create a queued job with an optional known total, including zero. */
  explicit JobLifecycle(std::optional<std::uint64_t> total = std::nullopt) noexcept { snapshot_.total_frames = total; }
  /** Copy the current state on the owning worker thread. */
  JobSnapshot snapshot() const noexcept { return snapshot_; }
  /** Transition a queued job to running. */
  bool start() noexcept {
    if (snapshot_.status != JobStatus::queued) return false;
    snapshot_.status = JobStatus::running; return true;
  }
  /** Update running progress; reject regression or counts beyond a known total. */
  bool progress(std::uint64_t frames) noexcept {
    if (snapshot_.status != JobStatus::running || frames < snapshot_.frames_processed ||
        (hasKnownTotal(snapshot_.total_frames) && frames > *snapshot_.total_frames)) return false;
    snapshot_.frames_processed = frames; return true;
  }
  /** Cancel queued or running work after the worker observes a request. */
  bool cancel() noexcept { return finish(JobStatus::cancelled); }
  /** Fail queued or running work. */
  bool fail() noexcept { return finish(JobStatus::failed); }
  /** Requeue cancelled or failed work and discard its partial progress. */
  bool retry() noexcept {
    if (!isTerminal(snapshot_.status) || snapshot_.status == JobStatus::complete) return false;
    snapshot_.status = JobStatus::queued;
    snapshot_.frames_processed = 0;
    return true;
  }
  /** Mark a running job complete; a known total must have been fully processed. */
  bool complete() noexcept {
    if (snapshot_.status != JobStatus::running ||
        (hasKnownTotal(snapshot_.total_frames) && snapshot_.frames_processed != *snapshot_.total_frames)) return false;
    snapshot_.status = JobStatus::complete; return true;
  }
private:
  bool finish(JobStatus status) noexcept {
    if (isTerminal(snapshot_.status)) return false;
    if (snapshot_.status != JobStatus::queued && snapshot_.status != JobStatus::running) return false;
    snapshot_.status = status; return true;
  }
  JobSnapshot snapshot_{};
};
}
