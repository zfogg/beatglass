#pragma once
#include <cstdint>
#include <optional>
#include <string>
namespace beatglass {
/** Lifecycle states shared by the library queue, UI, and persistence layer. */
enum class JobStatus { queued, running, cancelled, failed, complete };
/** Return whether a lifecycle transition is legal for a queued or active job.
 * Terminal states are intentionally not restartable; retry creates a new queued
 * lifecycle through the job owner so stale results cannot be revived in place.
 */
constexpr bool canTransition(JobStatus from, JobStatus to) noexcept {
  if (from == JobStatus::queued) return to == JobStatus::running || to == JobStatus::cancelled || to == JobStatus::failed;
  if (from == JobStatus::running) return to == JobStatus::cancelled || to == JobStatus::failed || to == JobStatus::complete;
  return false;
}
/** Return whether a job has reached a terminal state. */
constexpr bool isTerminal(JobStatus status) noexcept {
  return status == JobStatus::cancelled || status == JobStatus::failed || status == JobStatus::complete;
}
/** Cache identity fields. All fields participate in cache reuse decisions. */
struct AnalysisIdentity { std::string content_hash, engine_version, configuration_id, mode; };
/** Snapshot of bounded job progress. A zero total means an empty known input. */
struct JobProgress { std::uint64_t frames_processed{}, total_frames{}; JobStatus status{JobStatus::queued}; };
/** Return whether progress includes a meaningful total frame count. */
constexpr bool hasKnownTotal(const JobProgress& progress) noexcept { return progress.total_frames > 0; }
/** Return whether an optional total is present, including a known zero total. */
constexpr bool hasKnownTotal(const std::optional<std::uint64_t>& total) noexcept { return total.has_value(); }
/** Return whether a progress snapshot is internally consistent. */
constexpr bool isValidProgress(const JobProgress& progress) noexcept {
  return !hasKnownTotal(progress) || progress.frames_processed <= progress.total_frames;
}
}
