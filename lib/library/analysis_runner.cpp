#include "beatglass/library/analysis_runner.hpp"
namespace beatglass::library {
RetryableAnalysis::RetryableAnalysis(AnalysisRunner& runner, AnalysisOperation operation, int priority)
    : runner_(&runner), operation_(std::move(operation)), priority_(priority) {}
void RetryableAnalysis::cancel() {
  std::lock_guard lock(mutex_);
  if (handle_) handle_->cancel();
}
bool RetryableAnalysis::retry(int priority) {
  std::lock_guard lock(mutex_);
  if (priority != 0) priority_ = priority;
  if (handle_ && (handle_->state() == library_detail::TrackedState::queued || handle_->state() == library_detail::TrackedState::running)) return false;
  auto next = runner_->submit(operation_, priority_);
  if (!next) return false;
  handle_ = std::move(next);
  return true;
}
library_detail::TrackedState RetryableAnalysis::state() const {
  std::lock_guard lock(mutex_);
  return handle_ ? handle_->state() : library_detail::TrackedState::failed;
}
std::uint64_t RetryableAnalysis::frames() const {
  std::lock_guard lock(mutex_);
  return handle_ ? handle_->frames() : 0;
}
std::shared_ptr<library_detail::JobHandle> AnalysisRunner::submit(AnalysisOperation op,int priority){ return executor_.submit_tracked(priority,[op=std::move(op)](std::stop_token stop,const auto& progress){ if(stop.stop_requested()) return library_detail::TrackedState::cancelled; if(!op.valid()) return library_detail::TrackedState::failed; if(op.progress) op.progress(0,op.source.size_bytes); if(stop.stop_requested()) return library_detail::TrackedState::cancelled; auto verification=op.verify(stop); if(verification.outcome!=FileVerificationOutcome::match) return verification.outcome==FileVerificationOutcome::cancelled?library_detail::TrackedState::cancelled:library_detail::TrackedState::failed; auto result=op.serialize(); if(result.empty()) return library_detail::TrackedState::failed; progress(op.source.size_bytes); return library_detail::TrackedState::completed; }); }
std::shared_ptr<RetryableAnalysis> AnalysisRunner::submit_retryable(AnalysisOperation operation, int priority) {
  auto retained = std::shared_ptr<RetryableAnalysis>(new RetryableAnalysis(*this, std::move(operation), priority));
  retained->retry();
  return retained;
}
std::vector<std::shared_ptr<library_detail::JobHandle>> AnalysisRunner::submit_batch(std::vector<AnalysisOperation> operations,int priority){ std::vector<std::shared_ptr<library_detail::JobHandle>> accepted; accepted.reserve(operations.size()); for(auto& operation:operations){ auto handle=submit(std::move(operation),priority); if(!handle) break; accepted.push_back(std::move(handle)); } return accepted; }
}
