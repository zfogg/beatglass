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

std::shared_ptr<beatglass::library_detail::JobHandle> AnalysisJobBridge::submit(beatglass::library::AnalysisOperation op) {
  if (!op.valid()) {
    status_ = QStringLiteral("Failed");
    emit statusChanged();
    return {};
  }
  last_operation_ = op;
  frames_ = 0;
  auto progress = op.progress;
  op.progress = [this, progress](auto done, auto total) {
    if (progress) progress(done, total);
    QMetaObject::invokeMethod(this, [this, done] {
      frames_ = done;
      emit progressChanged();
    }, Qt::QueuedConnection);
  };
  handle_ = runner_.submit(std::move(op));
  status_ = handle_ ? QStringLiteral("Queued") : QStringLiteral("Failed");
  if (handle_) poll_timer_.start();
  emit statusChanged();
  return handle_;
}

bool AnalysisJobBridge::canCancel() const noexcept {
  if (!handle_) return false;
  const auto state = handle_->state();
  return state == beatglass::library_detail::TrackedState::queued || state == beatglass::library_detail::TrackedState::running;
}

bool AnalysisJobBridge::canRetry() const noexcept {
  if (!handle_ || last_operation_.source.path.empty()) return false;
  const auto state = handle_->state();
  return state == beatglass::library_detail::TrackedState::cancelled || state == beatglass::library_detail::TrackedState::failed;
}

void AnalysisJobBridge::cancel() {
  if (!canCancel()) return;
  handle_->cancel();
  poll();
}

void AnalysisJobBridge::retry() {
  if (!canRetry()) return;
  submit(last_operation_);
}

void AnalysisJobBridge::poll() {
  if (!handle_) return;
  const auto next = stateText(handle_->state());
  if (frames_ != handle_->frames()) { frames_ = handle_->frames(); emit progressChanged(); }
  if (status_ != next) { status_ = next; emit statusChanged(); }
  if (!canCancel()) poll_timer_.stop();
}

void AnalysisJobBridge::setStatus() { poll(); }
