#include "storage_recovery_bridge.hpp"
#include "beatglass/library/analysis_coordinator.hpp"
#include "beatglass/library/file_analysis_operation.hpp"
#include <QMetaObject>
#include <QTimer>
#include <algorithm>

class StorageRecoveryBridge::Worker final : public QObject {
  Q_OBJECT
public slots:
  void open(QString path) {
    beatglass::library::StorageConfig config{path.toStdString()};
    std::string diagnostic;
    const bool ok = coordinator_.open_storage(config, &diagnostic);
    emit result(ok, QString::fromStdString(ok ? "Storage open" : diagnostic));
    if (ok) refresh();
    else { emit runsChanged({}, {}); emit analysis(QStringLiteral("Analysis idle")); }
  }
  void close() {
    coordinator_.close_storage();
    emit runsChanged({}, {});
    emit analysis(QStringLiteral("Analysis idle"));
    emit result(true, QStringLiteral("Storage closed"));
  }
  void refresh() {
    const auto runs = coordinator_.storage().retryable_runs();
    QStringList ids;
    QStringList details;
    for (const auto& run : runs) { ids.push_back(QString::fromStdString(run.id)); details.push_back(QString::fromStdString(run.id + " | " + run.key.content + " | " + run.key.engine + " | " + run.key.config + " | " + run.key.mode + " | source=" + run.source_path + " | decoder=" + run.decoder_snapshot)); }
    emit analysis(runs.empty() ? QStringLiteral("Analysis idle") : QStringLiteral("Analysis recovery available"));
    emit runsChanged(ids, details);
  }
  void discard(QString id) { const bool ok = coordinator_.storage().discard_retryable(id.toStdString()); emit result(ok, ok ? QStringLiteral("Retryable run discarded") : QStringLiteral("Retryable run not found")); if (ok) refresh(); }
  void retry(QString runId) {
    const auto found = coordinator_.storage().retryable_run(runId.toStdString());
    if (!found || found->source_path.empty()) { emit result(false, QStringLiteral("Retry rejected: ") + QString::fromLatin1(beatglass::library::retry_rejection_message(beatglass::library::RetryRejectionReason::missing_source_snapshot))); return; }
    if (!beatglass::library::file_decoder_snapshot_compatible(found->decoder_snapshot)) { emit result(false, QStringLiteral("Retry rejected: ") + QString::fromLatin1(beatglass::library::retry_rejection_message(beatglass::library::RetryRejectionReason::incompatible_decoder_snapshot))); return; }
    beatglass::library::AnalysisIdentity identity{found->key.content, found->key.engine, found->key.config, found->key.mode};
    auto operation = beatglass::library::make_file_decode_operation(found->source_path, std::move(identity), found->decoder_snapshot);
    const auto rejection = coordinator_.retry_rejection_reason(runId.toStdString(), operation);
    // Includes RetryRejectionReason::stale_source_identity for an epoch mismatch.
    if (rejection != beatglass::library::RetryRejectionReason::none) { emit result(false, QStringLiteral("Retry rejected: ") + QString::fromLatin1(beatglass::library::retry_rejection_message(rejection))); return; }
    handle_ = coordinator_.resubmit_retryable(runId.toStdString(), std::move(operation));
    emit result(static_cast<bool>(handle_), handle_ ? QStringLiteral("Retry queued") : QStringLiteral("Retry rejected: source identity or metadata unavailable"));
    if (handle_) { emit analysis(QStringLiteral("Analysis queued")); QTimer::singleShot(50, this, &Worker::pollAnalysis); }
  }
  void findCached(QString content, QString engine, QString configuration, QString mode) {
    beatglass::AnalysisIdentity identity{content.toStdString(), engine.toStdString(), configuration.toStdString(), mode.toStdString()};
    std::string result; emit cached(coordinator_.storage().find_cached(identity, result), QString::fromStdString(result));
  }
  void invalidateCached(QString content, QString engine, QString configuration, QString mode) {
    beatglass::library::AnalysisIdentity identity{content.toStdString(), engine.toStdString(), configuration.toStdString(), mode.toStdString()};
    const bool removed = coordinator_.storage().invalidate_cached(identity);
    if (removed) emit cached(false, QString{});
    emit result(removed, removed ? QStringLiteral("Cached analysis invalidated") : QStringLiteral("No cached analysis found"));
  }
  void submitAnalysis(QString runId, QString path, QString contentHash, QString engine, QString configuration, QString mode) {
    beatglass::library::AnalysisIdentity identity{contentHash.toStdString(), engine.toStdString(), configuration.toStdString(), mode.toStdString()};
    auto operation = beatglass::library::make_file_decode_operation(path.toStdString(), std::move(identity));
    operation.publish = [this](const std::string& value) { emit analysisResult(QString::fromStdString(value)); };
    auto submission = coordinator_.submit_admitted(runId.toStdString(), std::move(operation));
    if (submission.cached) { emit analysisResult(QString::fromStdString(*submission.cached)); emit analysis(QStringLiteral("Cached analysis ready")); handle_.reset(); return; }
    handle_ = std::move(submission.handle); emit analysis(handle_ ? (submission.analysis_reserved ? QStringLiteral("Analysis queued") : QStringLiteral("Analysis rejected")) : QStringLiteral("Analysis rejected"));
    if (handle_) QTimer::singleShot(50, this, &Worker::pollAnalysis);
  }
  void cancelAnalysis() { if (handle_) { handle_->cancel(); pollAnalysis(); } }
  void retryAnalysis() { if (handle_ && handle_->retry()) { emit analysis(QStringLiteral("Analysis queued")); QTimer::singleShot(50, this, &Worker::pollAnalysis); } }
  void admitAnalysis(QString runId, QString content, QString engine, QString configuration, QString mode) {
    beatglass::AnalysisIdentity identity{content.toStdString(), engine.toStdString(), configuration.toStdString(), mode.toStdString()};
    const auto admission = coordinator_.admit_analysis(runId.toStdString(), identity);
    emit admitted(admission.analysis_reserved, QString::fromStdString(admission.cached.value_or(std::string{})));
  }
signals:
  void result(bool ok, QString status);
  void runsChanged(QStringList ids, QStringList details);
  void cached(bool hit, QString result);
  void analysis(QString status);
  void analysisError(QString error);
  void analysisProgress(qulonglong frames);
  void analysisResult(QString result);
  void admitted(bool reserved, QString cachedResult);
private:
  void pollAnalysis() {
    if (!handle_) return;
    using beatglass::library_detail::TrackedState;
    const auto state = handle_->state();
    emit analysisProgress(static_cast<qulonglong>(handle_->frames()));
    const auto status = state == TrackedState::queued ? "Analysis queued" : state == TrackedState::running ? "Analysis running" : state == TrackedState::completed ? "Analysis complete" : state == TrackedState::cancelled ? "Analysis cancelled" : "Analysis failed";
    emit analysis(QString::fromLatin1(status));
    if (state == TrackedState::failed) emit analysisError(QStringLiteral("Analysis failed; retryable work was preserved for recovery."));
    if (state == TrackedState::queued || state == TrackedState::running) QTimer::singleShot(50, this, &Worker::pollAnalysis); else refresh();
  }
  beatglass::library::AnalysisCoordinator coordinator_;
  std::shared_ptr<beatglass::library::RetryableAnalysis> handle_;
};

StorageRecoveryBridge::StorageRecoveryBridge(QObject* parent) : QObject(parent), worker_(new Worker) {
  worker_->moveToThread(&workerThread_);
  connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
  connect(worker_, &Worker::result, this, [this](bool ok, const QString& text) {
    status_ = ok ? text : QStringLiteral("Storage error: ") + text;
    if (!ok) {
      if (!retryableRunIds_.isEmpty()) { retryableRunIds_.clear(); emit retryableRunsChanged(); }
      if (!selectedRunId_.isEmpty()) { selectedRunId_.clear(); emit selectedRunChanged(); }
      if (!analysisError_.isEmpty()) { analysisError_.clear(); emit analysisErrorChanged(); }
      if (!resetStatus_.isEmpty()) { resetStatus_.clear(); emit resetStatusChanged(); }
      analysisStatus_ = QStringLiteral("Analysis idle"); emit analysisStatusChanged();
    }
    if (ok && (text == QStringLiteral("Retry queued") || text == QStringLiteral("Retryable run discarded") || text == QStringLiteral("Cached analysis invalidated"))) {
      if (!analysisError_.isEmpty()) { analysisError_.clear(); emit analysisErrorChanged(); }
    }
    if (ok && text == QStringLiteral("Retryable run discarded")) { resetStatus_ = QStringLiteral("Persisted recovery discarded"); emit resetStatusChanged(); }
    if (ok && text == QStringLiteral("Retry queued") && !resetStatus_.isEmpty()) { resetStatus_.clear(); emit resetStatusChanged(); }
    if (ok && text == QStringLiteral("Cached analysis invalidated")) {
      cacheHit_ = false; cachedResult_.clear(); emit cachedResultChanged();
      analysisStatus_ = QStringLiteral("Analysis idle · reanalysis required"); emit analysisStatusChanged();
    }
    emit statusChanged();
  }, Qt::QueuedConnection);
  connect(worker_, &Worker::runsChanged, this, [this](const QStringList& ids, const QStringList& details) {
    retryableRunIds_ = ids; retryableRunDetails_ = details;
    if (!selectedRunId_.isEmpty() && !retryableRunIds_.contains(selectedRunId_)) { selectedRunId_.clear(); emit selectedRunChanged(); }
    emit retryableRunsChanged();
  }, Qt::QueuedConnection);
  connect(worker_, &Worker::cached, this, [this](bool hit, const QString& result) {
    cacheHit_ = hit; cachedResult_ = result; emit cachedResultChanged();
  }, Qt::QueuedConnection);
  connect(worker_, &Worker::analysis, this, [this](const QString& text) { analysisStatus_ = text; const bool retryable = text == QStringLiteral("Analysis cancelled") || text == QStringLiteral("Analysis failed") || text == QStringLiteral("Analysis recovery available"); if (analysisRetryable_ != retryable) { analysisRetryable_ = retryable; emit analysisRetryableChanged(); } if (text != QStringLiteral("Analysis failed") && !analysisError_.isEmpty()) { analysisError_.clear(); emit analysisErrorChanged(); } emit analysisStatusChanged(); }, Qt::QueuedConnection);
  connect(worker_, &Worker::analysisError, this, [this](const QString& text) { analysisError_ = text; emit analysisErrorChanged(); }, Qt::QueuedConnection);
  connect(worker_, &Worker::analysisProgress, this, [this](qulonglong frames) { analysisFrames_ = frames; emit analysisProgressChanged(); }, Qt::QueuedConnection);
  connect(worker_, &Worker::analysisResult, this, [this](const QString& text) { analysisResult_ = text; emit analysisResultChanged(); }, Qt::QueuedConnection);
  connect(worker_, &Worker::admitted, this, [this](bool reserved, const QString& result) { analysisReserved_ = reserved; analysisAdmissionResult_ = result; emit analysisAdmissionChanged(); }, Qt::QueuedConnection);
  workerThread_.start();
}

StorageRecoveryBridge::~StorageRecoveryBridge() { close(); workerThread_.quit(); workerThread_.wait(); }
void StorageRecoveryBridge::setDatabasePath(const QString& path) {
  if (databasePath_ == path) return;
  if (!databasePath_.isEmpty() && !path.isEmpty()) close();
  databasePath_ = path; emit databasePathChanged();
  if (cacheHit_) { cacheHit_ = false; emit cachedResultChanged(); }
  if (!cachedResult_.isEmpty()) { cachedResult_.clear(); emit cachedResultChanged(); }
  if (!analysisResult_.isEmpty()) { analysisResult_.clear(); emit analysisResultChanged(); }
  if (analysisReserved_) { analysisReserved_ = false; emit analysisAdmissionChanged(); }
  if (!analysisAdmissionResult_.isEmpty()) { analysisAdmissionResult_.clear(); emit analysisAdmissionChanged(); }
  analysisStatus_ = QStringLiteral("Analysis idle"); emit analysisStatusChanged();
}
void StorageRecoveryBridge::open() {
  if (databasePath_.isEmpty()) { status_ = QStringLiteral("Storage error: database path is required"); emit statusChanged(); return; }
  if (!retryableRunIds_.isEmpty()) { retryableRunIds_.clear(); emit retryableRunsChanged(); }
  if (!selectedRunId_.isEmpty()) { selectedRunId_.clear(); emit selectedRunChanged(); }
  if (!analysisError_.isEmpty()) { analysisError_.clear(); emit analysisErrorChanged(); }
  if (!resetStatus_.isEmpty()) { resetStatus_.clear(); emit resetStatusChanged(); }
  QMetaObject::invokeMethod(worker_, "open", Qt::QueuedConnection, Q_ARG(QString, databasePath_));
  status_ = QStringLiteral("Opening storage"); emit statusChanged();
}
void StorageRecoveryBridge::close() {
  if (worker_) QMetaObject::invokeMethod(worker_, "close", Qt::QueuedConnection);
  if (!retryableRunIds_.isEmpty()) { retryableRunIds_.clear(); emit retryableRunsChanged(); }
  if (!selectedRunId_.isEmpty()) { selectedRunId_.clear(); emit selectedRunChanged(); }
  if (!analysisError_.isEmpty()) { analysisError_.clear(); emit analysisErrorChanged(); }
  if (!resetStatus_.isEmpty()) { resetStatus_.clear(); emit resetStatusChanged(); }
  if (analysisRetryable_) { analysisRetryable_ = false; emit analysisRetryableChanged(); }
  if (analysisFrames_ != 0) { analysisFrames_ = 0; emit analysisProgressChanged(); }
  analysisStatus_ = QStringLiteral("Analysis idle"); emit analysisStatusChanged();
  status_ = QStringLiteral("Storage closed"); emit statusChanged();
}
void StorageRecoveryBridge::refreshRetryableRuns() { if (worker_) QMetaObject::invokeMethod(worker_, "refresh", Qt::QueuedConnection); }
void StorageRecoveryBridge::selectRun(const QString& id) { if (selectedRunId_ == id) return; selectedRunId_ = id; emit selectedRunChanged(); }
void StorageRecoveryBridge::discardSelectedRun() { if (!selectedRunId_.isEmpty()) QMetaObject::invokeMethod(worker_, "discard", Qt::QueuedConnection, Q_ARG(QString, selectedRunId_)); }
void StorageRecoveryBridge::retrySelectedRun() { if (!selectedRunId_.isEmpty()) QMetaObject::invokeMethod(worker_, "retry", Qt::QueuedConnection, Q_ARG(QString, selectedRunId_)); }
void StorageRecoveryBridge::findCached(QString content, QString engine, QString configuration, QString mode) {
  if (worker_) QMetaObject::invokeMethod(worker_, "findCached", Qt::QueuedConnection, Q_ARG(QString, content), Q_ARG(QString, engine), Q_ARG(QString, configuration), Q_ARG(QString, mode));
}
void StorageRecoveryBridge::invalidateCached(QString content, QString engine, QString configuration, QString mode) {
  if (worker_) QMetaObject::invokeMethod(worker_, "invalidateCached", Qt::QueuedConnection, Q_ARG(QString, content), Q_ARG(QString, engine), Q_ARG(QString, configuration), Q_ARG(QString, mode));
}
void StorageRecoveryBridge::submitAnalysis(QString runId, QString path, QString contentHash, QString engine, QString configuration, QString mode) {
  if (worker_) QMetaObject::invokeMethod(worker_, "submitAnalysis", Qt::QueuedConnection, Q_ARG(QString, runId), Q_ARG(QString, path), Q_ARG(QString, contentHash), Q_ARG(QString, engine), Q_ARG(QString, configuration), Q_ARG(QString, mode));
}
void StorageRecoveryBridge::reanalyze(QString runId, QString path, QString contentHash, QString engine, QString configuration, QString mode) {
  invalidateCached(contentHash, engine, configuration, mode);
  submitAnalysis(std::move(runId), std::move(path), std::move(contentHash), std::move(engine), std::move(configuration), std::move(mode));
}
void StorageRecoveryBridge::cancelAnalysis() { if (worker_) QMetaObject::invokeMethod(worker_, "cancelAnalysis", Qt::QueuedConnection); }
void StorageRecoveryBridge::retryAnalysis() { if (worker_) QMetaObject::invokeMethod(worker_, "retryAnalysis", Qt::QueuedConnection); }
void StorageRecoveryBridge::admitAnalysis(QString runId, QString content, QString engine, QString configuration, QString mode) {
  if (worker_) QMetaObject::invokeMethod(worker_, "admitAnalysis", Qt::QueuedConnection, Q_ARG(QString, runId), Q_ARG(QString, content), Q_ARG(QString, engine), Q_ARG(QString, configuration), Q_ARG(QString, mode));
}
