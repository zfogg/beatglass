#include "analysis_job_bridge.hpp"
#include "beatglass/library/file_analysis_operation.hpp"
#include "beatglass/library/detail/file_fingerprint.hpp"

namespace {
constexpr std::size_t kMaxDisplayedResultBytes = 4096;
std::string displayedResult(std::string value) {
  if (value.size() <= kMaxDisplayedResultBytes) return value;
  value.resize(kMaxDisplayedResultBytes);
  value += "…[truncated]";
  return value;
}
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

bool AnalysisJobBridge::submitFile(const QString& path, const QString& contentHash,
                                   const QString& engineVersion, const QString& configurationId,
                                   const QString& mode) {
  if (engineVersion_ != engineVersion || configurationId_ != configurationId || analysisMode_ != mode) {
    engineVersion_ = engineVersion; configurationId_ = configurationId; analysisMode_ = mode; emit provenanceChanged();
  }
  beatglass::library::AnalysisIdentity identity;
  identity.content_hash = contentHash.toStdString(); identity.engine_version = engineVersion.toStdString();
  identity.configuration_id = configurationId.toStdString(); identity.mode = mode.toStdString();
  return static_cast<bool>(submit(beatglass::library::make_file_decode_operation(path.toStdString(), std::move(identity))));
}

bool AnalysisJobBridge::submitSelectedFile(const QString& path) {
  if (path.isEmpty()) return false;
  const auto fingerprint = beatglass::library_detail::fingerprint_file(path.toStdString());
  if (fingerprint.status != beatglass::library_detail::FingerprintStatus::ok) return false;
  return submitFile(path, QString::fromStdString(fingerprint.digest),
                    QStringLiteral("beatglass-desktop-v1"),
                    QStringLiteral("default-offline"));
}

int AnalysisJobBridge::submitSelectedFiles(const QStringList& paths) {
  std::vector<beatglass::library::AnalysisOperation> operations;
  operations.reserve(static_cast<std::size_t>(paths.size()));
  for (const auto& path : paths) {
    if (path.isEmpty()) continue;
    const auto fingerprint = beatglass::library_detail::fingerprint_file(path.toStdString());
    if (fingerprint.status != beatglass::library_detail::FingerprintStatus::ok) continue;
    beatglass::library::AnalysisIdentity identity;
    identity.content_hash = fingerprint.digest;
    identity.engine_version = "beatglass-desktop-v1";
    identity.configuration_id = "default-offline";
    identity.mode = "offline";
    operations.push_back(beatglass::library::make_file_decode_operation(path.toStdString(), std::move(identity)));
  }
  return submitBatch(std::move(operations));
}

std::shared_ptr<beatglass::library::RetryableAnalysis> AnalysisJobBridge::submit(beatglass::library::AnalysisOperation op) {
  if (!recoveryStatus_.isEmpty()) { recoveryStatus_.clear(); emit statusChanged(); }
  const auto sourcePath = QString::fromStdString(op.source.path.string());
  if (sourcePath_ != sourcePath) { sourcePath_ = sourcePath; emit sourceChanged(); }
  if (!op.valid()) {
    failureReason_ = QStringLiteral("Analysis request is missing valid source identity or worker callbacks.");
    emit failureChanged();
    if (frames_ != 0 || total_frames_ != 0 || progress_known_) { frames_ = 0; total_frames_ = 0; progress_known_ = false; emit progressChanged(); }
    if (!result_.isEmpty()) { result_.clear(); emit resultChanged(); }
    status_ = QStringLiteral("Failed");
    emit statusChanged();
    retryable_.reset(); return {};
  }
  frames_ = 0; total_frames_ = 0; progress_known_ = false;
  if (!failureReason_.isEmpty()) { failureReason_.clear(); emit failureChanged(); }
  result_.clear();
  emit resultChanged();
  for (const auto& operation : batch_) if (operation) operation->cancel();
  batch_.clear(); batchAccepted_ = batchRejected_ = batchResultsPublished_ = batchFailedCount_ = batchCancelledCount_ = batchCompletedCount_ = 0; batchLastResult_.clear(); batchLastSource_.clear(); emit batchChanged();
  auto progress = op.progress;
  auto publish = op.publish;
  op.publish = [weak_self = weak_self_, publish](const std::string& value) {
    if (publish) publish(value);
    if (!weak_self) return;
    const auto displayed = displayedResult(value);
    const auto result = QString::fromUtf8(displayed.data(), static_cast<int>(displayed.size()));
    QMetaObject::invokeMethod(weak_self, [weak_self, result] {
      if (!weak_self) return;
      weak_self->result_ = result;
      emit weak_self->resultChanged();
    }, Qt::QueuedConnection);
  };
  op.progress = [weak_self = weak_self_, progress](auto done, auto total) {
    if (progress) progress(done, total);
    if (!weak_self) return;
    QMetaObject::invokeMethod(weak_self, [weak_self, done, total] {
      if (!weak_self) return;
      weak_self->frames_ = done;
      weak_self->total_frames_ = total;
      weak_self->progress_known_ = total > 0;
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
  if (!result_.isEmpty()) { result_.clear(); emit resultChanged(); }
  if (!failureReason_.isEmpty()) { failureReason_.clear(); emit failureChanged(); }
  if (failureReason_.isEmpty()) { failureReason_ = QStringLiteral("Analysis cancelled before publication; retry is available."); emit failureChanged(); }
  if (status_ != QStringLiteral("Cancelled")) { status_ = QStringLiteral("Cancelled"); emit statusChanged(); }
  poll();
}

void AnalysisJobBridge::retry() {
  if (!canRetry()) return;
  if (retryable_->retry()) {
    if (!recoveryStatus_.isEmpty()) { recoveryStatus_.clear(); emit statusChanged(); }
    if (!failureReason_.isEmpty()) { failureReason_.clear(); emit failureChanged(); }
    frames_ = retryable_->frames();
    emit progressChanged();
    if (!result_.isEmpty()) { result_.clear(); emit resultChanged(); }
    status_ = QStringLiteral("Queued"); poll_timer_.start(); emit statusChanged(); poll();
  }
}

void AnalysisJobBridge::discard() {
  if (!retryable_ || canCancel()) return;
  retryable_.reset();
  poll_timer_.stop();
  if (!result_.isEmpty()) { result_.clear(); emit resultChanged(); }
  if (!failureReason_.isEmpty()) { failureReason_.clear(); emit failureChanged(); }
  if (frames_ != 0 || total_frames_ != 0 || progress_known_) { frames_ = 0; total_frames_ = 0; progress_known_ = false; emit progressChanged(); }
  if (status_ != QStringLiteral("Idle")) { status_ = QStringLiteral("Idle"); emit statusChanged(); }
  recoveryStatus_ = QStringLiteral("Single analysis recovery discarded"); emit statusChanged();
}

int AnalysisJobBridge::submitBatch(std::vector<beatglass::library::AnalysisOperation> operations, int priority) {
  if (!recoveryStatus_.isEmpty()) { recoveryStatus_.clear(); emit statusChanged(); }
  if (frames_ != 0 || total_frames_ != 0 || progress_known_) { frames_ = 0; total_frames_ = 0; progress_known_ = false; emit progressChanged(); }
  if (!failureReason_.isEmpty()) { failureReason_.clear(); emit failureChanged(); }
  if (retryable_ && canCancel()) retryable_->cancel();
  retryable_.reset();
  if (!sourcePath_.isEmpty()) { sourcePath_.clear(); emit sourceChanged(); }
  batch_.clear(); batchAccepted_ = batchRejected_ = batchResultsPublished_ = batchFailedCount_ = batchCancelledCount_ = batchCompletedCount_ = 0; batchLastResult_.clear(); batchLastSource_.clear();
  if (!result_.isEmpty()) { result_.clear(); emit resultChanged(); }
  for (auto& operation : operations) {
    if (static_cast<int>(batch_.size()) >= batchCapacity_) { ++batchRejected_; continue; }
    const auto sourcePath = QString::fromStdString(operation.source.path.string());
    auto publish = operation.publish;
    operation.publish = [weak_self = weak_self_, publish, sourcePath](const std::string& value) {
      if (publish) publish(value);
      if (!weak_self) return;
      const auto displayed = displayedResult(value);
      const auto result = QString::fromUtf8(displayed.data(), static_cast<int>(displayed.size()));
      QMetaObject::invokeMethod(weak_self, [weak_self, result, sourcePath] {
        if (!weak_self) return;
        ++weak_self->batchResultsPublished_;
        weak_self->batchLastResult_ = result;
        weak_self->batchLastSource_ = sourcePath;
        emit weak_self->batchChanged();
      }, Qt::QueuedConnection);
    };
    auto retained = runner_.submit_retryable(std::move(operation), priority);
    if (!retained) { ++batchRejected_; continue; }
    batch_.push_back(std::move(retained)); ++batchAccepted_;
  }
  emit batchChanged();
  if (!batch_.empty()) {
    if (status_ != QStringLiteral("Queued")) { status_ = QStringLiteral("Queued"); emit statusChanged(); }
    poll_timer_.start();
  } else if (batchRejected_ != 0 && status_ != QStringLiteral("Failed")) {
    status_ = QStringLiteral("Failed"); emit statusChanged();
  }
  return batchAccepted_;
}

void AnalysisJobBridge::cancelBatch() {
  for (const auto& operation : batch_) if (operation) operation->cancel();
  batchResultsPublished_ = batchFailedCount_ = batchCancelledCount_ = batchCompletedCount_ = 0; batchLastResult_.clear(); batchLastSource_.clear();
  if (!failureReason_.isEmpty()) { failureReason_.clear(); emit failureChanged(); }
  emit batchChanged();
  if (!result_.isEmpty()) { result_.clear(); emit resultChanged(); }
  if (!batch_.empty() && status_ != QStringLiteral("Cancelled")) { status_ = QStringLiteral("Cancelled"); emit statusChanged(); }
  poll();
}

bool AnalysisJobBridge::retryBatch() {
  bool accepted = false;
  for (const auto& operation : batch_) if (operation && operation->retry()) accepted = true;
  if (accepted) {
    if (!recoveryStatus_.isEmpty()) { recoveryStatus_.clear(); emit statusChanged(); }
    if (!failureReason_.isEmpty()) { failureReason_.clear(); emit failureChanged(); }
    if (frames_ != 0 || total_frames_ != 0 || progress_known_) { frames_ = 0; total_frames_ = 0; progress_known_ = false; emit progressChanged(); }
    batchResultsPublished_ = batchFailedCount_ = batchCancelledCount_ = batchCompletedCount_ = 0; batchLastResult_.clear(); batchLastSource_.clear();
    if (!result_.isEmpty()) { result_.clear(); emit resultChanged(); }
    poll_timer_.start(); emit statusChanged(); poll();
  }
  return accepted;
}

void AnalysisJobBridge::discardBatch() {
  bool terminal = !batch_.empty();
  for (const auto& operation : batch_) terminal = terminal && operation && !(
      operation->state() == beatglass::library_detail::TrackedState::queued ||
      operation->state() == beatglass::library_detail::TrackedState::running);
  if (!terminal) return;
  batch_.clear();
  batchAccepted_ = batchRejected_ = batchResultsPublished_ = batchFailedCount_ = batchCancelledCount_ = batchCompletedCount_ = 0;
  batchLastResult_.clear(); batchLastSource_.clear();
  poll_timer_.stop();
  if (!result_.isEmpty()) { result_.clear(); emit resultChanged(); }
  if (!failureReason_.isEmpty()) { failureReason_.clear(); emit failureChanged(); }
  if (frames_ != 0 || total_frames_ != 0 || progress_known_) { frames_ = 0; total_frames_ = 0; progress_known_ = false; emit progressChanged(); }
  if (status_ != QStringLiteral("Idle")) { status_ = QStringLiteral("Idle"); emit statusChanged(); }
  recoveryStatus_ = QStringLiteral("Batch analysis recovery discarded"); emit statusChanged();
  emit batchChanged();
}

void AnalysisJobBridge::poll() {
  if (!retryable_ && batch_.empty()) return;
  if (!retryable_ && !batch_.empty()) {
    bool active = false, failed = false, cancelled = false;
    int failed_count = 0, cancelled_count = 0, completed_count = 0;
    qulonglong total = 0;
    for (const auto& operation : batch_) {
      if (!operation) continue;
      const auto state = operation->state(); total += operation->frames();
      active = active || state == beatglass::library_detail::TrackedState::queued || state == beatglass::library_detail::TrackedState::running;
      failed = failed || state == beatglass::library_detail::TrackedState::failed;
      if (state == beatglass::library_detail::TrackedState::failed) ++failed_count;
      if (state == beatglass::library_detail::TrackedState::cancelled) ++cancelled_count;
      if (state == beatglass::library_detail::TrackedState::completed) ++completed_count;
      cancelled = cancelled || state == beatglass::library_detail::TrackedState::cancelled;
    }
    if (frames_ != total) { frames_ = total; emit progressChanged(); }
    if (batchFailedCount_ != failed_count) { batchFailedCount_ = failed_count; emit batchChanged(); }
    if (batchCancelledCount_ != cancelled_count) { batchCancelledCount_ = cancelled_count; emit batchChanged(); }
    if (batchCompletedCount_ != completed_count) { batchCompletedCount_ = completed_count; emit batchChanged(); }
    const auto next = active ? QStringLiteral("Running") : failed ? QStringLiteral("Failed") : cancelled ? QStringLiteral("Cancelled") : QStringLiteral("Complete");
    if (status_ != next) { status_ = next; emit statusChanged(); }
    if (next == QStringLiteral("Cancelled") && failureReason_.isEmpty()) { failureReason_ = QStringLiteral("Analysis batch cancelled before publication; retry is available."); emit failureChanged(); }
    if (next == QStringLiteral("Failed") && failureReason_.isEmpty()) { failureReason_ = QStringLiteral("One or more batch operations failed before publishing a verified result."); failureReason_ += QStringLiteral(" Retry is available."); emit failureChanged(); }
    if (!active) poll_timer_.stop();
    return;
  }
  const auto next = stateText(retryable_->state());
  if (frames_ != retryable_->frames()) { frames_ = retryable_->frames(); emit progressChanged(); }
  if (status_ != next) { status_ = next; emit statusChanged(); }
  if (next == QStringLiteral("Cancelled") && failureReason_.isEmpty()) { failureReason_ = QStringLiteral("Analysis cancelled before publication; retry is available."); emit failureChanged(); }
  if (next == QStringLiteral("Failed") && failureReason_.isEmpty()) { failureReason_ = QStringLiteral("Worker analysis failed before publishing a verified result; retry is available."); emit failureChanged(); }
  if (!canCancel()) poll_timer_.stop();
}

void AnalysisJobBridge::setStatus() { poll(); }
