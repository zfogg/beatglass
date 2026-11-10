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
std::shared_ptr<library_detail::JobHandle> AnalysisRunner::submit(AnalysisOperation op,int priority){ auto queued_recovery = op.recover_cancellation; return executor_.submit_tracked(priority,[op=std::move(op)](std::stop_token stop,const auto& progress) mutable { auto recover = [&] { try { if (op.recover_cancellation) op.recover_cancellation(); } catch (...) {} }; auto cancelled = [&] { recover(); return library_detail::TrackedState::cancelled; }; auto failed = [&] { recover(); return library_detail::TrackedState::failed; }; if(stop.stop_requested()) return cancelled(); if(!op.valid()) return library_detail::TrackedState::failed; auto report = op.progress; op.progress = [report, &progress](std::uint64_t processed, std::uint64_t total) { if(report) report(processed, total); progress(processed); }; op.progress(0,op.source.size_bytes); if(stop.stop_requested()) return cancelled(); FileVerificationResult verification; try { verification = op.verify(stop); } catch (...) { return failed(); } if(verification.outcome!=FileVerificationOutcome::match) return verification.outcome==FileVerificationOutcome::cancelled?cancelled():failed(); if(stop.stop_requested()) return cancelled(); auto decode_progress = [&op](std::uint64_t processed) { op.progress(processed, op.source.size_bytes); }; std::string result; try { result = op.decode ? op.decode(stop, decode_progress) : op.serialize(); } catch (...) { return failed(); } if(stop.stop_requested()) return cancelled(); if(result.empty()) return failed(); try { if(op.publish_transaction && !op.publish_transaction(result, stop)) return stop.stop_requested() ? cancelled() : failed(); if(op.publish) op.publish(result); } catch (...) { return failed(); } op.progress(op.source.size_bytes,op.source.size_bytes); return library_detail::TrackedState::completed; }, std::move(queued_recovery)); }
std::shared_ptr<RetryableAnalysis> AnalysisRunner::submit_retryable(AnalysisOperation operation, int priority) {
  auto retained = std::shared_ptr<RetryableAnalysis>(new RetryableAnalysis(*this, std::move(operation), priority));
  retained->retry();
  return retained;
}
std::vector<std::shared_ptr<library_detail::JobHandle>> AnalysisRunner::submit_batch(std::vector<AnalysisOperation> operations,int priority){ std::vector<std::shared_ptr<library_detail::JobHandle>> accepted; accepted.reserve(operations.size()); for(auto& operation:operations){ auto handle=submit(std::move(operation),priority); if(!handle) break; accepted.push_back(std::move(handle)); } return accepted; }
}
