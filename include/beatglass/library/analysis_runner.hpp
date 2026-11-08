#pragma once
#include "beatglass/library/analysis_operation.hpp"
#include "beatglass/library/detail/job_executor.hpp"
#include <memory>
#include <mutex>
#include <cstdint>
#include <vector>
namespace beatglass::library {
class AnalysisRunner;
/** A retained analysis request that can be submitted again after cancellation or failure. */
class RetryableAnalysis {
public:
  /** Request cancellation of the current attempt, if one is queued or running. */
  void cancel();
  /** Submit a fresh attempt when the previous attempt is terminal; return whether it was accepted. */
  bool retry(int priority = 0);
  /** Return the current attempt state, or failed before the first accepted attempt. */
  library_detail::TrackedState state() const;
  /** Return progress from the current attempt. */
  std::uint64_t frames() const;
private:
  friend class AnalysisRunner;
  RetryableAnalysis(AnalysisRunner& runner, AnalysisOperation operation, int priority);
  AnalysisRunner* runner_;
  AnalysisOperation operation_;
  int priority_;
  mutable std::mutex mutex_;
  std::shared_ptr<library_detail::JobHandle> handle_;
};
/** Worker-owned adapter that executes an AnalysisOperation and retains its JobHandle. Empty serialized results fail closed before persistence. */
class AnalysisRunner {
public:
  explicit AnalysisRunner(library_detail::JobExecutor& executor):executor_(executor){}
  std::shared_ptr<library_detail::JobHandle> submit(AnalysisOperation operation, int priority = 0);
  /** Retain an operation so a cancelled or failed attempt can be retried without reconstructing source identity. */
  std::shared_ptr<RetryableAnalysis> submit_retryable(AnalysisOperation operation, int priority = 0);
  /** Submit a bounded batch in input order; returns only accepted operations when queue capacity is exhausted. */
  std::vector<std::shared_ptr<library_detail::JobHandle>> submit_batch(std::vector<AnalysisOperation> operations, int priority = 0);
private: library_detail::JobExecutor& executor_;
};
}
