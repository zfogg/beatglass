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
  /** Convert the worker snapshot to the shared bounded progress contract. */
  JobProgress progress() const noexcept {
    return JobProgress{frames_processed, total_frames.value_or(0), status};
  }
  /** Return a UI-safe fraction in [0, 1], or no value when total is unknown. */
  std::optional<double> fraction() const noexcept {
    if (!total_frames.has_value()) return std::nullopt;
    if (*total_frames == 0) return 1.0;
    return static_cast<double>(frames_processed) / static_cast<double>(*total_frames);
  }
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
    if (!canTransition(snapshot_.status, JobStatus::running)) return false;
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
    if (!canTransition(snapshot_.status, JobStatus::complete) ||
        (hasKnownTotal(snapshot_.total_frames) && snapshot_.frames_processed != *snapshot_.total_frames)) return false;
    snapshot_.status = JobStatus::complete; return true;
  }
private:
  bool finish(JobStatus status) noexcept {
    if (!canTransition(snapshot_.status, status)) return false;
    snapshot_.status = status; return true;
  }
  JobSnapshot snapshot_{};
};
}
