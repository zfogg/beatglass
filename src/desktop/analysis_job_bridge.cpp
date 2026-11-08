#include "analysis_job_bridge.hpp"

namespace {
QString stateText(beatglass::library_detail::TrackedState state) {
  using beatglass::library_detail::TrackedState;
  switch (state) {
    case TrackedState::queued: return QStringLiteral("Queued");
    case TrackedState::running: return QStringLiteral("Running");
    case TrackedState::completed: return QStringLiteral("Complete");
    case TrackedState::cancelled: return QStringLiteral("Cancelled");
    case TrackedState::failed: return QStringLiteral("Failed");
  }
  return QStringLiteral("Unknown");
}
}

AnalysisJobBridge::AnalysisJobBridge(beatglass::library_detail::JobExecutor& executor, QObject* parent)
    : QObject(parent), runner_(executor) {
  poll_timer_.setInterval(50);
  poll_timer_.setSingleShot(false);
  connect(&poll_timer_, &QTimer::timeout, this, &AnalysisJobBridge::poll);
}

std::shared_ptr<beatglass::library::RetryableAnalysis> AnalysisJobBridge::submit(beatglass::library::AnalysisOperation op) {
  if (!op.valid()) {
    status_ = QStringLiteral("Failed");
    emit statusChanged();
    retryable_.reset(); return {};
  }
  frames_ = 0;
  auto progress = op.progress;
  op.progress = [this, progress](auto done, auto total) {
    if (progress) progress(done, total);
    QMetaObject::invokeMethod(this, [this, done] {
      frames_ = done;
      emit progressChanged();
    }, Qt::QueuedConnection);
  };
  retryable_ = runner_.submit_retryable(std::move(op));
  status_ = retryable_ && (retryable_->state() == beatglass::library_detail::TrackedState::queued || retryable_->state() == beatglass::library_detail::TrackedState::running) ? QStringLiteral("Queued") : QStringLiteral("Failed");
  if (retryable_) poll_timer_.start();
  emit statusChanged();
  return retryable_;
}

bool AnalysisJobBridge::canCancel() const noexcept {
  if (!retryable_) return false;
  const auto state = retryable_->state();
  return state == beatglass::library_detail::TrackedState::queued || state == beatglass::library_detail::TrackedState::running;
}

bool AnalysisJobBridge::canRetry() const noexcept {
  if (!retryable_) return false;
  const auto state = retryable_->state();
  return state == beatglass::library_detail::TrackedState::cancelled || state == beatglass::library_detail::TrackedState::failed;
}

void AnalysisJobBridge::cancel() {
  if (!canCancel()) return;
  retryable_->cancel();
  poll();
}

void AnalysisJobBridge::retry() {
  if (!canRetry()) return;
  if (retryable_->retry()) { frames_ = retryable_->frames(); status_ = QStringLiteral("Queued"); poll_timer_.start(); emit statusChanged(); }
}

void AnalysisJobBridge::poll() {
  if (!retryable_) return;
  const auto next = stateText(retryable_->state());
  if (frames_ != retryable_->frames()) { frames_ = retryable_->frames(); emit progressChanged(); }
  if (status_ != next) { status_ = next; emit statusChanged(); }
  if (!canCancel()) poll_timer_.stop();
}

void AnalysisJobBridge::setStatus() { poll(); }
