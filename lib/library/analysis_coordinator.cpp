#include "beatglass/library/analysis_coordinator.hpp"
#include "beatglass/library/analysis_commit.hpp"
namespace beatglass::library {
std::shared_ptr<RetryableAnalysis> AnalysisCoordinator::submit(std::string run_id, AnalysisOperation operation, int priority) {
  if (!storage_.cache() || run_id.empty() || !operation.valid()) return {};
  const auto admission = storage_.admit_analysis(run_id, operation.identity);
  if (!admission.analysis_reserved && !admission.cached) return {};
  if (admission.cached) {
    const auto cached = *admission.cached;
    operation.decode = [cached](std::stop_token, const std::function<void(std::uint64_t)>&) { return cached; };
  }
  operation.publish_transaction = make_transactional_publisher(*storage_.cache(), std::move(run_id), operation.identity, operation.verify);
  return runner_.submit_retryable(std::move(operation), priority);
}
AnalysisCoordinator::Submission AnalysisCoordinator::submit_admitted(std::string run_id, AnalysisOperation operation, int priority) {
  if (!storage_.cache() || run_id.empty() || !operation.valid()) return {};
  const auto submitted_run = run_id;
  const auto submitted_identity = operation.identity;
  auto admission = admit_analysis(*storage_.cache(), run_id, operation.identity);
  if (admission.cached) return {submitted_run, submitted_identity, std::move(admission.cached), {}, false};
  if (!admission.analysis_reserved) return {};
  return {submitted_run, submitted_identity, {}, submit(std::move(run_id), std::move(operation), priority), true};
}
}
