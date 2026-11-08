#include "beatglass/library/analysis_coordinator.hpp"
#include "beatglass/library/analysis_commit.hpp"
namespace beatglass::library {
std::shared_ptr<RetryableAnalysis> AnalysisCoordinator::submit(std::string run_id, AnalysisOperation operation, int priority) {
  if (!storage_.cache() || run_id.empty() || !operation.valid()) return {};
  operation.publish_transaction = make_transactional_publisher(*storage_.cache(), std::move(run_id), operation.identity, operation.verify);
  return runner_.submit_retryable(std::move(operation), priority);
}
AnalysisCoordinator::Submission AnalysisCoordinator::submit_admitted(std::string run_id, AnalysisOperation operation, int priority) {
  if (!storage_.cache() || run_id.empty() || !operation.valid()) return {};
  auto admission = admit_analysis(*storage_.cache(), run_id, operation.identity);
  if (admission.cached) return {std::move(admission.cached), {}, false};
  if (!admission.analysis_reserved) return {};
  return {{}, submit(std::move(run_id), std::move(operation), priority), true};
}
}
