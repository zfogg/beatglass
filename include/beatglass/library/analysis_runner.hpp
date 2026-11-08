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
  /**
   * Bind the runner to an existing worker executor; the executor must outlive
   * this runner and owns queue capacity, ordering, and shutdown.
   */
  explicit AnalysisRunner(library_detail::JobExecutor& executor):executor_(executor){}
  /**
   * Submit one operation to the worker queue. The returned handle is empty
   * when the bounded queue rejects the work; execution and callbacks remain
   * off the audio and GUI threads. Decoder progress is forwarded to both the
   * operation callback and the retained handle before publication.
   */
  std::shared_ptr<library_detail::JobHandle> submit(AnalysisOperation operation, int priority = 0);
  /**
   * Retain an operation so a cancelled or failed attempt can be retried
   * without reconstructing source identity. The returned controller owns its
   * operation copy and must not outlive the runner or its executor.
   */
  std::shared_ptr<RetryableAnalysis> submit_retryable(AnalysisOperation operation, int priority = 0);
  /**
   * Submit a bounded batch in input order. The result contains only accepted
   * operations when queue capacity is exhausted; input operations are copied
   * or moved into worker-owned jobs before this call returns.
   */
  std::vector<std::shared_ptr<library_detail::JobHandle>> submit_batch(std::vector<AnalysisOperation> operations, int priority = 0);
private: library_detail::JobExecutor& executor_;
};
}
