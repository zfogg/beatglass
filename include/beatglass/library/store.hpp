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
/** Result of checking the same opened input used by analysis against its original evidence. */
enum class FileVerificationOutcome { match, changed, unverifiable, cancelled, error };
/** Verification result; diagnostic describes an error or inability to verify. */
struct FileVerificationResult { FileVerificationOutcome outcome{FileVerificationOutcome::error}; std::string diagnostic; };
/** Map verification to a terminal publication outcome; a match permits publication to continue.
 * This does not start or commit a transaction; call on the database worker
 * before the commit decision. Cancellation after commit starts is too late.
 */
inline std::optional<PublishOutcome> verification_failure(FileVerificationOutcome outcome) noexcept {
  switch (outcome) {
    case FileVerificationOutcome::match: return std::nullopt;
    case FileVerificationOutcome::changed:
    case FileVerificationOutcome::unverifiable: return PublishOutcome::stale_input;
    case FileVerificationOutcome::cancelled: return PublishOutcome::cancelled;
    case FileVerificationOutcome::error: return PublishOutcome::error;
  }
  return PublishOutcome::error;
}
/** Synchronous, worker-invoked revalidation of the opened analysis source.
 * The caller keeps this object and its source evidence alive until publish
 * returns. Its implementation must be safe on the database worker; no decoder
 * or file handle may be accessed concurrently unless its owner permits it.
 * A successful match is limited by the platform's documented filesystem races.
 */
class FileVersionVerifier {
public:
  virtual ~FileVersionVerifier() = default;
  /** Recheck retained pre-analysis evidence; observe cancellation cooperatively.
   * A blocking filesystem call may not be interruptible. Return cancelled if
   * cancellation is observed before verification completes, not stale_input.
   */
  virtual FileVerificationResult verify(std::stop_token cancel) = 0;
};
/** Recovery result after reopening storage: committed, absent, or still indeterminate due to error. */
enum class RecoveryOutcome { committed, not_found, error };
/** Recovery status and optional diagnostic for errors. */
struct RecoveryResult { RecoveryOutcome outcome{RecoveryOutcome::error}; std::string diagnostic; };
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
  /**
   * Find a validated cached result, miss, cancellation or error. The returned
   * snapshot owns its value; the caller retains the source evidence only for
   * the duration of this worker call.
   */
  virtual FindResult find(const FileVersion&, const AnalysisIdentity&, std::stop_token) = 0;
  /** Stage, invoke verifier on this worker, then publish at the serialized commit point.
   * Pass the token to verification. Changed or unverifiable evidence returns
   * stale_input, observed cancellation returns cancelled, and verification errors
   * return error. run_id must be unique and durable across retries/reopening.
   * The caller retains verifier and source evidence until this call returns.
   */
  virtual PublishResult publish(const FileVersion& before, const CachedAnalysis&,
                                const std::string& run_id, FileVersionVerifier& verifier,
                                std::stop_token) = 0;
  /**
   * Query a unique run ID after reopening; `not_found` means no committed run
   * with that ID. Recovery is a worker operation and does not alter source
   * files or silently convert an indeterminate commit into success.
   */
  virtual RecoveryResult recover(const std::string& run_id) = 0;
};
/** Worker-local commit decision; neither a transaction nor a cross-thread arbiter.
 * Deliver cancellation to the worker and call begin_commit at its commit point.
 */
class CommitDecision {
public:
  /**
   * Choose cancellation if requested before commit; otherwise enter commit.
   * Only the first decision succeeds; this object is worker-local and is not
   * safe for concurrent calls.
   */
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
/** Compatibility alias for library-facing callers; source versions remain core-owned values. */
namespace beatglass::library { using FileVersion = ::beatglass::FileVersion; }
