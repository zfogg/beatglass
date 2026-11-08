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
  result_.clear();
  emit resultChanged();
  batch_.clear(); batchAccepted_ = batchRejected_ = 0; emit batchChanged();
  auto progress = op.progress;
  auto publish = op.publish;
  op.publish = [weak_self = weak_self_, publish](const std::string& value) {
    if (publish) publish(value);
    if (!weak_self) return;
    const auto result = QString::fromUtf8(value.data(), static_cast<int>(value.size()));
    QMetaObject::invokeMethod(weak_self, [weak_self, result] {
      if (!weak_self) return;
      weak_self->result_ = result;
      emit weak_self->resultChanged();
    }, Qt::QueuedConnection);
  };
  op.progress = [weak_self = weak_self_, progress](auto done, auto total) {
    if (progress) progress(done, total);
    if (!weak_self) return;
    QMetaObject::invokeMethod(weak_self, [weak_self, done] {
      if (!weak_self) return;
      weak_self->frames_ = done;
      emit weak_self->progressChanged();
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
  if (retryable_->retry()) {
    frames_ = retryable_->frames();
    if (!result_.isEmpty()) { result_.clear(); emit resultChanged(); }
    status_ = QStringLiteral("Queued"); poll_timer_.start(); emit statusChanged();
  }
}

int AnalysisJobBridge::submitBatch(std::vector<beatglass::library::AnalysisOperation> operations, int priority) {
  batch_.clear(); batchAccepted_ = batchRejected_ = batchResultsPublished_ = 0;
  for (auto& operation : operations) {
    if (static_cast<int>(batch_.size()) >= batchCapacity_) { ++batchRejected_; continue; }
    auto publish = operation.publish;
    operation.publish = [weak_self = weak_self_, publish](const std::string& value) {
      if (publish) publish(value);
      if (!weak_self) return;
      QMetaObject::invokeMethod(weak_self, [weak_self] {
        if (!weak_self) return;
        ++weak_self->batchResultsPublished_;
        emit weak_self->batchChanged();
      }, Qt::QueuedConnection);
    };
    auto retained = runner_.submit_retryable(std::move(operation), priority);
    if (!retained) { ++batchRejected_; continue; }
    batch_.push_back(std::move(retained)); ++batchAccepted_;
  }
  emit batchChanged();
  if (!batch_.empty()) poll_timer_.start();
  return batchAccepted_;
}

void AnalysisJobBridge::cancelBatch() {
  for (const auto& operation : batch_) if (operation) operation->cancel();
  poll();
}

void AnalysisJobBridge::retryBatch() {
  bool accepted = false;
  for (const auto& operation : batch_) if (operation && operation->retry()) accepted = true;
  if (accepted) {
    if (!result_.isEmpty()) { result_.clear(); emit resultChanged(); }
    poll_timer_.start(); emit statusChanged();
  }
}

void AnalysisJobBridge::poll() {
  if (!retryable_ && batch_.empty()) return;
  if (!retryable_ && !batch_.empty()) {
    bool active = false, failed = false, cancelled = false;
    qulonglong total = 0;
    for (const auto& operation : batch_) {
      if (!operation) continue;
      const auto state = operation->state(); total += operation->frames();
      active = active || state == beatglass::library_detail::TrackedState::queued || state == beatglass::library_detail::TrackedState::running;
      failed = failed || state == beatglass::library_detail::TrackedState::failed;
      cancelled = cancelled || state == beatglass::library_detail::TrackedState::cancelled;
    }
    if (frames_ != total) { frames_ = total; emit progressChanged(); }
    const auto next = active ? QStringLiteral("Running") : failed ? QStringLiteral("Failed") : cancelled ? QStringLiteral("Cancelled") : QStringLiteral("Complete");
    if (status_ != next) { status_ = next; emit statusChanged(); }
    if (!active) poll_timer_.stop();
    return;
  }
  const auto next = stateText(retryable_->state());
  if (frames_ != retryable_->frames()) { frames_ = retryable_->frames(); emit progressChanged(); }
  if (status_ != next) { status_ = next; emit statusChanged(); }
  if (!canCancel()) poll_timer_.stop();
}

void AnalysisJobBridge::setStatus() { poll(); }
