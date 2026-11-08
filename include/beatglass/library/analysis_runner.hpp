#pragma once
#include "beatglass/library/analysis_operation.hpp"
#include "beatglass/library/detail/job_executor.hpp"
#include <memory>
#include <vector>
namespace beatglass::library {
/** Worker-owned adapter that executes an AnalysisOperation and retains its JobHandle. Empty serialized results fail closed before persistence. */
class AnalysisRunner {
public:
  explicit AnalysisRunner(library_detail::JobExecutor& executor):executor_(executor){}
  std::shared_ptr<library_detail::JobHandle> submit(AnalysisOperation operation, int priority = 0);
  /** Submit a bounded batch in input order; returns only accepted operations when queue capacity is exhausted. */
  std::vector<std::shared_ptr<library_detail::JobHandle>> submit_batch(std::vector<AnalysisOperation> operations, int priority = 0);
private: library_detail::JobExecutor& executor_;
};
}
