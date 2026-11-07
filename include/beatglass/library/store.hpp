#pragma once
#include "beatglass/core/results.hpp"
#include "beatglass/library/jobs.hpp"
#include <cstdint>
#include <filesystem>
#include <optional>
#include <stop_token>
#include <string>
namespace beatglass {
/** File identity evidence; hash policy and platform-specific version proof remain implementation decisions. */
struct FileVersion { std::filesystem::path path; std::uint64_t size_bytes{}; std::int64_t modified_ns{}; std::string content_hash; };
/** Cached identity and value snapshot. */
struct CachedAnalysis { AnalysisIdentity identity; Snapshot result; };
/** Lookup outcomes; a miss and cancellation are not storage errors. */
enum class FindOutcome { hit, miss, cancelled, error };
/** Lookup value: result is present only for hit; diagnostic describes errors. */
struct FindResult { FindOutcome outcome{FindOutcome::error}; std::optional<CachedAnalysis> result; std::string diagnostic; };
/** Publication outcomes. An indeterminate commit is an error, not committed or cancelled. */
enum class PublishOutcome { committed, cancelled, stale_input, error };
/** Publication result; diagnostic describes errors including an indeterminate commit. */
struct PublishResult { PublishOutcome outcome{PublishOutcome::error}; std::string diagnostic; };
/** Worker-owned storage boundary, not a database implementation.
 * Connections and writes belong to a database worker, never the GUI or audio
 * callback. Keys include validated content identity, engine and algorithm
 * versions, configuration and mode. Recheck opened-file identity and version
 * during publication; unreliable or changed evidence returns stale_input and
 * preserves the previous completed result. Stat/hash checks alone cannot close
 * every filesystem race; implementations must document their remaining race.
 * Stage and atomically replace within a transaction. Cancellation observed by
 * the worker before commit begins rolls back and returns cancelled; once commit
 * begins, resolve as committed or error, even if cancellation is later requested.
 * Never mark a job complete before successful commit. If commit outcome is
 * unknown, return an indeterminate error and recover by unique run ID after
 * reopening. This interface does not itself enforce transaction semantics.
 */
class AnalysisStore {
public:
  virtual ~AnalysisStore() = default;
  /** Find a validated cached result, miss, cancellation or error. */
  virtual FindResult find(const FileVersion&, const AnalysisIdentity&, std::stop_token) = 0;
  /** Revalidate and publish, applying the worker-serialized commit decision above. */
  virtual PublishResult publish(const FileVersion& before, const FileVersion& after,
                                const CachedAnalysis&, std::stop_token) = 0;
};
/** Worker-local commit decision; neither a transaction nor a cross-thread arbiter.
 * Deliver cancellation to the worker and call begin_commit at its commit point.
 */
class CommitDecision {
public:
  /** Choose cancellation if requested before commit; otherwise enter commit. Only the first decision succeeds. */
  bool begin_commit(bool cancellation_requested) noexcept {
    if (state_ != State::pending) return false;
    state_ = cancellation_requested ? State::cancelled : State::committing;
    return state_ == State::committing;
  }
  /** Resolve a begun commit; later cancellation cannot change the outcome. */
  std::optional<PublishOutcome> finish(bool success) noexcept {
    if (state_ != State::committing) return std::nullopt;
    state_ = State::finished;
    return success ? PublishOutcome::committed : PublishOutcome::error;
  }
  /** Whether cancellation won before commit. */
  bool cancelled() const noexcept { return state_ == State::cancelled; }
private:
  enum class State { pending, cancelled, committing, finished };
  State state_{State::pending};
};
}
