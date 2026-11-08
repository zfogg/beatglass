#pragma once
#include "beatglass/library/analysis_runner.hpp"
#include "beatglass/library/storage_owner.hpp"
namespace beatglass::library {
/** Worker-owned coordinator joining analysis execution to transactional storage. */
class AnalysisCoordinator final { public:
  explicit AnalysisCoordinator(std::size_t capacity = 2) : executor_(capacity), runner_(executor_) {}
  AnalysisCoordinator(const AnalysisCoordinator&) = delete;
  bool open_storage(const StorageConfig& config, std::string* diagnostic = nullptr) { return storage_.open(config, diagnostic); }
  void close_storage() noexcept { storage_.close(); }
  /** Submit an operation; publication and revalidation stay on this worker boundary. */
  std::shared_ptr<RetryableAnalysis> submit(std::string run_id, AnalysisOperation operation, int priority = 0);
  StorageOwner& storage() noexcept { return storage_; }
private: library_detail::JobExecutor executor_; AnalysisRunner runner_; StorageOwner storage_; };
}
