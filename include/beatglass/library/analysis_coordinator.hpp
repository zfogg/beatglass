#pragma once
#include "beatglass/library/analysis_runner.hpp"
#include "beatglass/library/storage_owner.hpp"
#include "beatglass/library/analysis_admission.hpp"
#include <algorithm>
namespace beatglass::library {
/** Explain why persisted retry reconstruction was rejected before work starts. */
enum class RetryRejectionReason { none, missing_source_snapshot, incompatible_decoder_snapshot, stale_source_identity };
inline const char* retry_rejection_message(RetryRejectionReason reason) noexcept {
  switch (reason) {
    case RetryRejectionReason::missing_source_snapshot: return "persisted source snapshot unavailable";
    case RetryRejectionReason::incompatible_decoder_snapshot: return "incompatible decoder snapshot";
    case RetryRejectionReason::stale_source_identity: return "stale source identity";
    default: return "";
  }
}
/** Worker-owned coordinator joining analysis execution to transactional storage. */
class AnalysisCoordinator final { public:
  /** Library-facing lifecycle snapshot; identity remains attached to every queued outcome. */
  struct Submission {
    std::string run_id;
    AnalysisIdentity identity;
    std::optional<std::string> cached;
    std::shared_ptr<RetryableAnalysis> handle;
    bool analysis_reserved{};
    library_detail::TrackedState state() const noexcept {
      return handle ? handle->state() : (cached ? library_detail::TrackedState::completed : library_detail::TrackedState::failed);
    }
  };
  explicit AnalysisCoordinator(std::size_t capacity = 2) : executor_(capacity), runner_(executor_) {}
  AnalysisCoordinator(const AnalysisCoordinator&) = delete;
  bool open_storage(const StorageConfig& config, std::string* diagnostic = nullptr) { return storage_.open(config, diagnostic); }
  void close_storage() noexcept { storage_.close(); }
  /** Submit an operation; admission persists source path/decoder snapshot and publication/revalidation stay on this worker boundary. */
  std::shared_ptr<RetryableAnalysis> submit(std::string run_id, AnalysisOperation operation, int priority = 0);
  /** Re-submit a persisted retryable run with a freshly reconstructed source operation. */
  std::shared_ptr<RetryableAnalysis> resubmit_retryable(std::string run_id, AnalysisOperation operation, int priority = 0) {
    if (run_id.empty() || run_id.find_first_not_of(" \t\r\n") == std::string::npos) return {};
    if (!storage_.retryable_run(run_id)) return {};
    if (retry_rejection_reason(run_id, operation) != RetryRejectionReason::none) return {};
    return submit(std::move(run_id), std::move(operation), priority);
  }
  /** Classify persisted retry compatibility before reconstructing decoder work. */
  RetryRejectionReason retry_rejection_reason(const std::string& run_id, const AnalysisOperation& operation) const {
    const auto found = storage_.retryable_run(run_id);
    if (!found || found->source_path.empty()) return RetryRejectionReason::missing_source_snapshot;
    if (found->key.content != operation.identity.content_hash || found->key.engine != operation.identity.engine_version || found->key.config != operation.identity.configuration_id || found->key.mode != operation.identity.mode) return RetryRejectionReason::stale_source_identity;
    if (found->decoder_snapshot != operation.decoder_snapshot) return RetryRejectionReason::incompatible_decoder_snapshot;
    return RetryRejectionReason::none;
  }
  Submission submit_admitted(std::string run_id, AnalysisOperation operation, int priority = 0);
  /** Return cache-hit or reserved-run state through the coordinator-owned model boundary. */
  CacheAdmission admit_analysis(const std::string& run_id, const AnalysisIdentity& identity) {
    return storage_.admit_analysis(run_id, identity);
  }
  StorageOwner& storage() noexcept { return storage_; }
private: library_detail::JobExecutor executor_; AnalysisRunner runner_; StorageOwner storage_; };
}
