#include "beatglass/library/analysis_coordinator.hpp"
#include "beatglass/library/analysis_commit.hpp"
namespace beatglass::library {
std::shared_ptr<RetryableAnalysis> AnalysisCoordinator::submit(std::string run_id, AnalysisOperation operation, int priority) {
  if (!storage_.cache() || run_id.empty() || run_id.size() > 4096 || run_id.find('\0') != std::string::npos || run_id.find_first_not_of(" \t\r\n") == std::string::npos || !operation.valid()) return {};
  const auto admission = storage_.admit_analysis(run_id, operation.identity, operation.source.path.string(), operation.decoder_snapshot);
  if (!admission.decoder_compatible || (!admission.analysis_reserved && !admission.cached)) return {};
  if (admission.cached) {
    const auto cached = *admission.cached;
    operation.decode = [cached](std::stop_token, const std::function<void(std::uint64_t)>&) { return cached; };
    // A cache hit is already committed; execute the normal completion path
    // without attempting to publish the completed run a second time.
    operation.publish_transaction = {};
  }
  const auto recovery_id = run_id;
  const library_detail::CacheKey recovery_key{operation.identity.content_hash, operation.identity.engine_version, operation.identity.configuration_id, operation.identity.mode};
  operation.recover_cancellation = [cache = storage_.cache(), recovery_id, recovery_key] {
    if (cache) cache->mark_retryable(recovery_id, recovery_key);
  };
  if (!admission.cached) {
    operation.publish_transaction = make_transactional_publisher(*storage_.cache(), std::move(run_id), operation.identity, operation.verify);
  }
  return runner_.submit_retryable(std::move(operation), priority);
}
AnalysisCoordinator::Submission AnalysisCoordinator::submit_admitted(std::string run_id, AnalysisOperation operation, int priority) {
  if (!storage_.cache() || run_id.empty() || run_id.size() > 4096 || run_id.find('\0') != std::string::npos || run_id.find_first_not_of(" \t\r\n") == std::string::npos || !operation.valid()) return {};
  const auto submitted_run = run_id;
  const auto submitted_identity = operation.identity;
  auto admission = ::beatglass::library::admit_analysis(*storage_.cache(), run_id, operation.identity, operation.source.path.string(), operation.decoder_snapshot);
  if (admission.cached) return {submitted_run, submitted_identity, std::move(admission.cached), {}, false};
  if (!admission.decoder_compatible || !admission.analysis_reserved) return {};
  return {submitted_run, submitted_identity, {}, submit(std::move(run_id), std::move(operation), priority), true};
}
}
