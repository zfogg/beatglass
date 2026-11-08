#pragma once
#include "beatglass/library/analysis_runner.hpp"
#include "beatglass/library/storage_owner.hpp"
#include "beatglass/library/analysis_admission.hpp"
namespace beatglass::library {
/** Worker-owned coordinator joining analysis execution to transactional storage. */
class AnalysisCoordinator final { public:
  struct Submission { std::optional<std::string> cached; std::shared_ptr<RetryableAnalysis> handle; bool analysis_reserved{}; };
  explicit AnalysisCoordinator(std::size_t capacity = 2) : executor_(capacity), runner_(executor_) {}
  AnalysisCoordinator(const AnalysisCoordinator&) = delete;
  bool open_storage(const StorageConfig& config, std::string* diagnostic = nullptr) { return storage_.open(config, diagnostic); }
  void close_storage() noexcept { storage_.close(); }
  /** Submit an operation; publication and revalidation stay on this worker boundary. */
  std::shared_ptr<RetryableAnalysis> submit(std::string run_id, AnalysisOperation operation, int priority = 0);
  Submission submit_admitted(std::string run_id, AnalysisOperation operation, int priority = 0);
  /** Return cache-hit or reserved-run state through the coordinator-owned model boundary. */
  CacheAdmission admit_analysis(const std::string& run_id, const AnalysisIdentity& identity) {
    return storage_.admit_analysis(run_id, identity);
  }
  StorageOwner& storage() noexcept { return storage_; }
private: library_detail::JobExecutor executor_; AnalysisRunner runner_; StorageOwner storage_; };
}
