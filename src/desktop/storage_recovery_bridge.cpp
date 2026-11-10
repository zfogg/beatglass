#include "storage_recovery_bridge.hpp"
#include "beatglass/library/analysis_coordinator.hpp"
#include "beatglass/library/file_analysis_operation.hpp"
#include "beatglass/library/details.hpp"
#include <QMetaObject>
#include <QTimer>
#include <algorithm>


class StorageRecoveryBridge::Worker final : public QObject {
  Q_OBJECT
public slots:
  void open(QString path) {
    // Clear the previous database's selection before asynchronous transition work begins.
    handle_.reset();
    emit runsChanged({}, {});
    emit analysis(QStringLiteral("Analysis idle"));
    beatglass::library::StorageConfig config{path.toStdString()};
    std::string diagnostic;
    bool ok = false;
    try { ok = coordinator_.open_storage(config, &diagnostic); }
    catch (const std::exception& error) { diagnostic = error.what(); }
    catch (...) { diagnostic = "unknown storage error"; }
    emit result(ok, QString::fromStdString(ok ? "Storage open" : diagnostic));
    if (ok) refresh();
    else { emit runsChanged({}, {}); emit analysis(QStringLiteral("Analysis idle")); }
  }
  void close() {
    handle_.reset();
    try { coordinator_.close_storage(); emit result(true, QStringLiteral("Storage closed")); }
    catch (const std::exception& error) { emit result(false, QStringLiteral("Storage close failed: ") + QString::fromUtf8(error.what())); }
    catch (...) { emit result(false, QStringLiteral("Storage close failed: unknown storage error")); }
    emit runsChanged({}, {}); emit analysis(QStringLiteral("Analysis idle"));
  }
  void refresh() {
    std::vector<beatglass::library_detail::RetryableRun> runs;
    try { runs = coordinator_.storage().retryable_runs(); }
    catch (const std::exception& error) { emit result(false, QStringLiteral("Recovery refresh failed: ") + QString::fromUtf8(error.what())); emit runsChanged({}, {}); emit analysis(QStringLiteral("Analysis idle")); return; }
    catch (...) { emit result(false, QStringLiteral("Recovery refresh failed: unknown storage error")); emit runsChanged({}, {}); emit analysis(QStringLiteral("Analysis idle")); return; }
    QStringList ids;
    QStringList details;
    for (const auto& run : runs) { ids.push_back(QString::fromStdString(run.id)); details.push_back(QString::fromStdString(run.id + " | " + run.key.content + " | " + run.key.engine + " | " + run.key.config + " | " + run.key.mode + " | source=" + run.source_path + " | decoder=" + run.decoder_snapshot)); }
    emit analysis(runs.empty() ? QStringLiteral("Analysis idle") : QStringLiteral("Analysis recovery available"));
    emit runsChanged(ids, details);
  }
  void discard(QString id) { if (handle_ && (handle_->state() == beatglass::library_detail::TrackedState::queued || handle_->state() == beatglass::library_detail::TrackedState::running)) { emit result(false, QStringLiteral("Retryable run is active")); return; } try { const bool ok = coordinator_.storage().discard_retryable(id.toStdString()); emit result(ok, ok ? QStringLiteral("Retryable run discarded") : QStringLiteral("Retryable run not found")); if (ok) { handle_.reset(); emit analysis(QStringLiteral("Analysis idle")); } refresh(); } catch (const std::exception& error) { emit result(false, QStringLiteral("Recovery discard rejected: ") + QString::fromUtf8(error.what())); refresh(); } catch (...) { emit result(false, QStringLiteral("Recovery discard rejected: unknown storage error")); refresh(); } }
  void discardBatch(QStringList ids) {
    if (ids.isEmpty()) { emit result(false, QStringLiteral("No retryable recovery runs selected")); refresh(); return; }
    if (handle_ && (handle_->state() == beatglass::library_detail::TrackedState::queued || handle_->state() == beatglass::library_detail::TrackedState::running)) { emit result(false, QStringLiteral("Retryable run is active")); emit analysis(QStringLiteral("Discard rejected: retryable run is active")); return; }
    try {
      std::vector<std::string> values; for (const auto& id : ids) { const auto normalized = id.trimmed(); const auto value = normalized.toStdString(); if (!value.empty() && std::find(values.begin(), values.end(), value) == values.end()) values.push_back(value); }
      if (values.empty()) { emit result(false, QStringLiteral("No retryable recovery runs selected")); refresh(); return; }
      const auto removed = coordinator_.storage().discard_retryable(values); const bool complete = removed == values.size();
      emit result(complete, QStringLiteral("%1 of %2 retryable recovery runs discarded").arg(static_cast<qulonglong>(removed)).arg(static_cast<qulonglong>(values.size())));
      if (removed) { handle_.reset(); emit analysis(QStringLiteral("Analysis idle")); } refresh();
    } catch (const std::exception& error) { emit result(false, QStringLiteral("Recovery discard rejected: ") + QString::fromUtf8(error.what())); refresh(); }
    catch (...) { emit result(false, QStringLiteral("Recovery discard rejected: unknown storage error")); refresh(); }
  }
  void retry(QString runId) {
    runId = runId.trimmed();
    if (runId.isEmpty()) { emit result(false, QStringLiteral("Retry rejected: no persisted run selected")); refresh(); return; }
    std::optional<beatglass::library_detail::RetryableRun> found;
    try { found = coordinator_.storage().retryable_run(runId.toStdString()); }
    catch (const std::exception& error) { emit result(false, QStringLiteral("Retry rejected: ") + QString::fromUtf8(error.what())); refresh(); return; }
    catch (...) { emit result(false, QStringLiteral("Retry rejected: unknown storage error")); refresh(); return; }
    if (!found || found->source_path.empty()) { emit result(false, QStringLiteral("Retry rejected: ") + QString::fromLatin1(beatglass::library::retry_rejection_message(beatglass::library::RetryRejectionReason::missing_source_snapshot))); refresh(); return; }
    try { if (!beatglass::library::file_decoder_snapshot_compatible(found->decoder_snapshot)) { emit result(false, QStringLiteral("Retry rejected: ") + QString::fromLatin1(beatglass::library::retry_rejection_message(beatglass::library::RetryRejectionReason::incompatible_decoder_snapshot))); refresh(); return; } }
    catch (const std::exception& error) { emit result(false, QStringLiteral("Retry rejected: ") + QString::fromUtf8(error.what())); refresh(); return; }
    beatglass::library::AnalysisIdentity identity{found->key.content, found->key.engine, found->key.config, found->key.mode};
    beatglass::library::AnalysisOperation operation;
    try { operation = beatglass::library::make_file_decode_operation(found->source_path, std::move(identity), found->decoder_snapshot); }
    catch (const std::exception& error) { emit result(false, QStringLiteral("Retry rejected: ") + QString::fromUtf8(error.what())); refresh(); return; }
    beatglass::library::RetryRejectionReason rejection;
    try { rejection = coordinator_.retry_rejection_reason(runId.toStdString(), operation); }
    catch (const std::exception& error) { emit result(false, QStringLiteral("Retry rejected: ") + QString::fromUtf8(error.what())); refresh(); return; }
    // Includes RetryRejectionReason::stale_source_identity for an epoch mismatch.
    if (rejection != beatglass::library::RetryRejectionReason::none) { emit result(false, QStringLiteral("Retry rejected: ") + QString::fromLatin1(beatglass::library::retry_rejection_message(rejection))); refresh(); return; }
    try { handle_ = coordinator_.resubmit_retryable(runId.toStdString(), std::move(operation)); }
    catch (const std::exception& error) { handle_.reset(); emit result(false, QStringLiteral("Retry rejected: ") + QString::fromUtf8(error.what())); refresh(); return; }
    emit result(static_cast<bool>(handle_), handle_ ? QStringLiteral("Retry queued") : QStringLiteral("Retry rejected: source identity or metadata unavailable"));
    if (!handle_) refresh();
    if (handle_) { refresh(); emit analysis(QStringLiteral("Analysis queued")); QTimer::singleShot(50, this, &Worker::pollAnalysis); }
  }
  void findCached(QString content, QString engine, QString configuration, QString mode) {
    beatglass::AnalysisIdentity identity{content.toStdString(), engine.toStdString(), configuration.toStdString(), mode.toStdString()};
    try { std::string result; emit cached(coordinator_.storage().find_cached(identity, result), QString::fromStdString(result)); }
    catch (const std::exception& error) { emit cached(false, QString{}); emit result(false, QStringLiteral("Cache lookup rejected: ") + QString::fromUtf8(error.what())); }
    catch (...) { emit cached(false, QString{}); emit result(false, QStringLiteral("Cache lookup rejected: unknown storage error")); }
  }
  void invalidateCached(QString content, QString engine, QString configuration, QString mode) {
    beatglass::library::AnalysisIdentity identity{content.toStdString(), engine.toStdString(), configuration.toStdString(), mode.toStdString()};
    try { const bool removed = coordinator_.storage().invalidate_cached(identity);
      if (removed) emit cached(false, QString{});
      emit result(removed, removed ? QStringLiteral("Cached analysis invalidated") : QStringLiteral("No cached analysis found"));
    } catch (const std::exception& error) { emit cached(false, QString{}); emit result(false, QStringLiteral("Cache invalidation rejected: ") + QString::fromUtf8(error.what())); }
    catch (...) { emit cached(false, QString{}); emit result(false, QStringLiteral("Cache invalidation rejected: unknown storage error")); }
  }
  void submitAnalysis(QString runId, QString path, QString contentHash, QString engine, QString configuration, QString mode) {
    const auto details = beatglass::library::inspect_file(path.toStdString());
    if (!details || !details->analysis_supported) {
      emit result(false, QStringLiteral("Analysis rejected: decoder does not support this audio format"));
      emit analysis(QStringLiteral("Analysis rejected"));
      return;
    }
    try {
      beatglass::library::AnalysisIdentity identity{contentHash.toStdString(), engine.toStdString(), configuration.toStdString(), mode.toStdString()};
      auto operation = beatglass::library::make_file_decode_operation(path.toStdString(), std::move(identity));
      operation.publish = [this](const std::string& value) { emit analysisResult(QString::fromStdString(value)); };
      auto submission = coordinator_.submit_admitted(runId.toStdString(), std::move(operation));
      if (submission.cached) { emit analysisResult(QString::fromStdString(*submission.cached)); emit analysis(QStringLiteral("Cached analysis ready")); handle_.reset(); return; }
      handle_ = std::move(submission.handle); emit analysis(handle_ ? (submission.analysis_reserved ? QStringLiteral("Analysis queued") : QStringLiteral("Analysis rejected")) : QStringLiteral("Analysis rejected"));
      if (handle_) QTimer::singleShot(50, this, &Worker::pollAnalysis);
    } catch (const std::exception& error) { handle_.reset(); emit result(false, QStringLiteral("Analysis submission rejected: ") + QString::fromUtf8(error.what())); emit analysis(QStringLiteral("Analysis rejected")); }
    catch (...) { handle_.reset(); emit result(false, QStringLiteral("Analysis submission rejected: unknown error")); emit analysis(QStringLiteral("Analysis rejected")); }
  }
  void cancelAnalysis() {
    if (!handle_) return;
    try { emit analysisProgress(0); emit analysis(QStringLiteral("Analysis cancelling")); handle_->cancel(); pollAnalysis(); }
    catch (const std::exception& error) { handle_.reset(); emit result(false, QStringLiteral("Analysis cancellation rejected: ") + QString::fromUtf8(error.what())); emit analysis(QStringLiteral("Analysis cancellation rejected")); }
    catch (...) { handle_.reset(); emit result(false, QStringLiteral("Analysis cancellation rejected: unknown error")); emit analysis(QStringLiteral("Analysis cancellation rejected")); }
  }
  void retryAnalysis() {
    try { if (handle_ && handle_->retry()) { emit analysisProgress(0); emit analysis(QStringLiteral("Analysis queued")); QTimer::singleShot(50, this, &Worker::pollAnalysis); }
      else if (handle_) { emit result(false, QStringLiteral("Analysis retry rejected: job is no longer retryable")); emit analysis(QStringLiteral("Analysis retry rejected")); } }
    catch (const std::exception& error) { handle_.reset(); emit result(false, QStringLiteral("Analysis retry rejected: ") + QString::fromUtf8(error.what())); emit analysis(QStringLiteral("Analysis retry rejected")); }
    catch (...) { handle_.reset(); emit result(false, QStringLiteral("Analysis retry rejected: unknown error")); emit analysis(QStringLiteral("Analysis retry rejected")); }
  }
  void admitAnalysis(QString runId, QString content, QString engine, QString configuration, QString mode) {
    try {
      beatglass::AnalysisIdentity identity{content.toStdString(), engine.toStdString(), configuration.toStdString(), mode.toStdString()};
      const auto admission = coordinator_.admit_analysis(runId.toStdString(), identity);
      emit admitted(admission.analysis_reserved, QString::fromStdString(admission.cached.value_or(std::string{})));
    } catch (const std::exception& error) { emit admitted(false, QString{}); emit result(false, QStringLiteral("Analysis admission rejected: ") + QString::fromUtf8(error.what())); emit analysis(QStringLiteral("Analysis admission rejected")); }
    catch (...) { emit admitted(false, QString{}); emit result(false, QStringLiteral("Analysis admission rejected: unknown error")); emit analysis(QStringLiteral("Analysis admission rejected")); }
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
    if (state != TrackedState::queued && state != TrackedState::running)
      emit result(state == TrackedState::completed, QString::fromLatin1(status));
    if (state == TrackedState::cancelled || state == TrackedState::failed)
      emit analysisProgress(0);
    if (state == TrackedState::failed) emit analysisError(QStringLiteral("Analysis failed; retryable work was preserved for recovery."));
    if (state == TrackedState::queued || state == TrackedState::running) QTimer::singleShot(50, this, &Worker::pollAnalysis); else { refresh(); if (state == TrackedState::completed) handle_.reset(); }
  }
  beatglass::library::AnalysisCoordinator coordinator_;
  std::shared_ptr<beatglass::library::RetryableAnalysis> handle_;
};

StorageRecoveryBridge::StorageRecoveryBridge(QObject* parent) : QObject(parent), worker_(new Worker) {
  worker_->moveToThread(&workerThread_);
  connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
  connect(worker_, &Worker::result, this, [this](bool ok, const QString& text) {
    const bool opened = ok && text == QStringLiteral("Storage open");
    const bool closed = text == QStringLiteral("Storage closed") || (storageTransitionPending_ && !ok);
    if (storageTransitionPending_) { storageTransitionPending_ = false; emit storageStateChanged(); }
    if ((opened && !storageOpen_) || (closed && storageOpen_)) { storageOpen_ = opened; emit storageStateChanged(); }
    status_ = ok ? text : QStringLiteral("Storage error: ") + text;
    if (!ok) {
      if (!retryableRunIds_.isEmpty() || !retryableRunDetails_.isEmpty()) { retryableRunIds_.clear(); retryableRunDetails_.clear(); emit retryableRunsChanged(); }
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
      if (analysisRetryable_) { analysisRetryable_ = false; emit analysisRetryableChanged(); }
      if (!analysisError_.isEmpty()) { analysisError_.clear(); emit analysisErrorChanged(); }
      if (!analysisResult_.isEmpty()) { analysisResult_.clear(); emit analysisResultChanged(); }
      if (analysisAdmission_) { analysisAdmission_ = false; emit analysisAdmissionChanged(); }
      if (!analysisAdmissionResult_.isEmpty()) { analysisAdmissionResult_.clear(); emit analysisAdmissionChanged(); }
      analysisStatus_ = QStringLiteral("Analysis idle · reanalysis required"); emit analysisStatusChanged();
    }
    if (ok && (text == QStringLiteral("Retryable run discarded") || text.contains(QStringLiteral("retryable recovery runs discarded")))) { analysisStatus_ = QStringLiteral("Analysis idle"); emit analysisStatusChanged(); }
    emit statusChanged();
  }, Qt::QueuedConnection);
  connect(worker_, &Worker::runsChanged, this, [this](const QStringList& ids, const QStringList& details) {
    retryableRunIds_ = ids; retryableRunDetails_ = details;
    if (!selectedRunId_.isEmpty() && !retryableRunIds_.contains(selectedRunId_)) { selectedRunId_.clear(); emit selectedRunChanged(); }
    if (status_ == QStringLiteral("Refreshing persisted recovery")) { status_ = QStringLiteral("Persisted recovery refreshed"); emit statusChanged(); }
    emit retryableRunsChanged();
  }, Qt::QueuedConnection);
  connect(worker_, &Worker::cached, this, [this](bool hit, const QString& result) {
    cacheHit_ = hit; cachedResult_ = result; emit cachedResultChanged();
    analysisStatus_ = hit ? QStringLiteral("Cached analysis ready") : QStringLiteral("Cache lookup complete"); emit analysisStatusChanged();
    const auto displayed = hit ? result : QString{};
    if (analysisResult_ != displayed) { analysisResult_ = displayed; emit analysisResultChanged(); }
    if (analysisRetryable_) { analysisRetryable_ = false; emit analysisRetryableChanged(); }
    if (!analysisError_.isEmpty()) { analysisError_.clear(); emit analysisErrorChanged(); }
    if (!hit && !analysisResult_.isEmpty()) { analysisResult_.clear(); emit analysisResultChanged(); }
    if (analysisFrames_ != 0) { analysisFrames_ = 0; emit analysisProgressChanged(); }
    if (analysisAdmission_) { analysisAdmission_ = false; emit analysisAdmissionChanged(); }
    if (!analysisAdmissionResult_.isEmpty()) { analysisAdmissionResult_.clear(); emit analysisAdmissionChanged(); }
  }, Qt::QueuedConnection);
  connect(worker_, &Worker::analysis, this, [this](const QString& text) { analysisStatus_ = text; const bool retryable = text == QStringLiteral("Analysis cancelled") || text == QStringLiteral("Analysis failed") || text == QStringLiteral("Analysis recovery available"); if (analysisRetryable_ != retryable) { analysisRetryable_ = retryable; emit analysisRetryableChanged(); } if (text != QStringLiteral("Analysis failed") && !analysisError_.isEmpty()) { analysisError_.clear(); emit analysisErrorChanged(); } if ((text == QStringLiteral("Analysis cancelled") || text == QStringLiteral("Analysis failed")) && !analysisResult_.isEmpty()) { analysisResult_.clear(); emit analysisResultChanged(); } emit analysisStatusChanged(); }, Qt::QueuedConnection);
  connect(worker_, &Worker::analysisError, this, [this](const QString& text) { analysisError_ = text; emit analysisErrorChanged(); }, Qt::QueuedConnection);
  connect(worker_, &Worker::analysisProgress, this, [this](qulonglong frames) { analysisFrames_ = frames; emit analysisProgressChanged(); }, Qt::QueuedConnection);
  connect(worker_, &Worker::analysisResult, this, [this](const QString& text) { analysisResult_ = text; emit analysisResultChanged(); }, Qt::QueuedConnection);
  connect(worker_, &Worker::admitted, this, [this](bool reserved, const QString& result) { analysisReserved_ = reserved; analysisAdmissionResult_ = result; cacheHit_ = !result.isEmpty(); cachedResult_ = result; analysisStatus_ = !result.isEmpty() ? QStringLiteral("Cached analysis ready") : reserved ? QStringLiteral("Analysis queued") : QStringLiteral("Analysis rejected"); if (analysisRetryable_) { analysisRetryable_ = false; emit analysisRetryableChanged(); } if (!analysisError_.isEmpty()) { analysisError_.clear(); emit analysisErrorChanged(); } if (analysisFrames_ != 0) { analysisFrames_ = 0; emit analysisProgressChanged(); } emit analysisAdmissionChanged(); emit cachedResultChanged(); emit analysisStatusChanged(); }, Qt::QueuedConnection);
  workerThread_.start();
}

StorageRecoveryBridge::~StorageRecoveryBridge() { close(); workerThread_.quit(); workerThread_.wait(); }
void StorageRecoveryBridge::setDatabasePath(const QString& path) {
  const auto normalized = path.trimmed();
  if (databasePath_ == normalized) return;
  if (storageTransitionPending_) { status_ = QStringLiteral("Storage transition already in progress"); emit statusChanged(); return; }
  if (!databasePath_.isEmpty()) close();
  databasePath_ = normalized; emit databasePathChanged();
  if (cacheHit_) { cacheHit_ = false; emit cachedResultChanged(); }
  if (!cachedResult_.isEmpty()) { cachedResult_.clear(); emit cachedResultChanged(); }
  if (!analysisResult_.isEmpty()) { analysisResult_.clear(); emit analysisResultChanged(); }
  if (analysisReserved_) { analysisReserved_ = false; emit analysisAdmissionChanged(); }
  if (!analysisAdmissionResult_.isEmpty()) { analysisAdmissionResult_.clear(); emit analysisAdmissionChanged(); }
  analysisStatus_ = QStringLiteral("Analysis idle"); emit analysisStatusChanged();
}
void StorageRecoveryBridge::open() {
  if (databasePath_.isEmpty()) { status_ = QStringLiteral("Storage error: database path is required"); emit statusChanged(); return; }
  if (storageTransitionPending_) { status_ = QStringLiteral("Storage transition already in progress"); emit statusChanged(); return; }
  if (storageOpen_) { status_ = QStringLiteral("Storage already open"); emit statusChanged(); return; }
  if (!retryableRunIds_.isEmpty() || !retryableRunDetails_.isEmpty()) { retryableRunIds_.clear(); retryableRunDetails_.clear(); emit retryableRunsChanged(); }
  if (!selectedRunId_.isEmpty()) { selectedRunId_.clear(); emit selectedRunChanged(); }
  if (!analysisError_.isEmpty()) { analysisError_.clear(); emit analysisErrorChanged(); }
  if (!resetStatus_.isEmpty()) { resetStatus_.clear(); emit resetStatusChanged(); }
  if (cacheHit_) { cacheHit_ = false; emit cachedResultChanged(); }
  if (!cachedResult_.isEmpty()) { cachedResult_.clear(); emit cachedResultChanged(); }
  if (analysisReserved_) { analysisReserved_ = false; emit analysisAdmissionChanged(); }
  if (!analysisAdmissionResult_.isEmpty()) { analysisAdmissionResult_.clear(); emit analysisAdmissionChanged(); }
  if (analysisRetryable_) { analysisRetryable_ = false; emit analysisRetryableChanged(); }
  if (analysisFrames_ != 0) { analysisFrames_ = 0; emit analysisProgressChanged(); }
  if (!analysisResult_.isEmpty()) { analysisResult_.clear(); emit analysisResultChanged(); }
  analysisStatus_ = QStringLiteral("Analysis idle"); emit analysisStatusChanged();
  if (!worker_) { status_ = QStringLiteral("Storage error: worker unavailable"); emit statusChanged(); return; }
  storageTransitionPending_ = true; emit storageStateChanged();
  QMetaObject::invokeMethod(worker_, "open", Qt::QueuedConnection, Q_ARG(QString, databasePath_));
  status_ = QStringLiteral("Opening storage"); emit statusChanged();
}
void StorageRecoveryBridge::close() {
  if (storageTransitionPending_) { status_ = QStringLiteral("Storage transition already in progress"); emit statusChanged(); return; }
  if (!storageOpen_ && !storageTransitionPending_) { status_ = QStringLiteral("Storage already closed"); emit statusChanged(); return; }
  storageTransitionPending_ = true; emit storageStateChanged();
  if (worker_) QMetaObject::invokeMethod(worker_, "close", Qt::QueuedConnection);
  if (!retryableRunIds_.isEmpty() || !retryableRunDetails_.isEmpty()) { retryableRunIds_.clear(); retryableRunDetails_.clear(); emit retryableRunsChanged(); }
  if (!selectedRunId_.isEmpty()) { selectedRunId_.clear(); emit selectedRunChanged(); }
  if (!analysisError_.isEmpty()) { analysisError_.clear(); emit analysisErrorChanged(); }
  if (!resetStatus_.isEmpty()) { resetStatus_.clear(); emit resetStatusChanged(); }
  if (cacheHit_) { cacheHit_ = false; emit cachedResultChanged(); }
  if (!cachedResult_.isEmpty()) { cachedResult_.clear(); emit cachedResultChanged(); }
  if (analysisReserved_) { analysisReserved_ = false; emit analysisAdmissionChanged(); }
  if (!analysisAdmissionResult_.isEmpty()) { analysisAdmissionResult_.clear(); emit analysisAdmissionChanged(); }
  if (analysisRetryable_) { analysisRetryable_ = false; emit analysisRetryableChanged(); }
  if (analysisFrames_ != 0) { analysisFrames_ = 0; emit analysisProgressChanged(); }
  if (!analysisResult_.isEmpty()) { analysisResult_.clear(); emit analysisResultChanged(); }
  analysisStatus_ = QStringLiteral("Analysis idle"); emit analysisStatusChanged();
  status_ = worker_ ? QStringLiteral("Storage closed") : QStringLiteral("Storage error: worker unavailable"); emit statusChanged();
}
void StorageRecoveryBridge::refreshRetryableRuns() { if (!storageOpen_) { status_ = QStringLiteral("Storage error: storage is not open"); emit statusChanged(); return; } if (worker_) { status_ = QStringLiteral("Refreshing persisted recovery"); emit statusChanged(); QMetaObject::invokeMethod(worker_, "refresh", Qt::QueuedConnection); } else { status_ = QStringLiteral("Storage error: worker unavailable"); emit statusChanged(); } }
void StorageRecoveryBridge::selectRun(const QString& id) { const auto normalized = id.trimmed(); if (selectedRunId_ == normalized) return; selectedRunId_ = normalized; emit selectedRunChanged(); status_ = normalized.isEmpty() ? QStringLiteral("Persisted recovery selection cleared") : QStringLiteral("Persisted recovery selected"); emit statusChanged(); }
void StorageRecoveryBridge::discardRuns(const QStringList& ids) { if (ids.isEmpty()) { status_ = QStringLiteral("Discard rejected: no persisted runs selected"); emit statusChanged(); return; } if (worker_) { if (!analysisResult_.isEmpty()) { analysisResult_.clear(); emit analysisResultChanged(); } if (analysisFrames_ != 0) { analysisFrames_ = 0; emit analysisProgressChanged(); } if (analysisRetryable_) { analysisRetryable_ = false; emit analysisRetryableChanged(); } if (!analysisError_.isEmpty()) { analysisError_.clear(); emit analysisErrorChanged(); } status_ = QStringLiteral("Discarding persisted recovery"); emit statusChanged(); QMetaObject::invokeMethod(worker_, "discardBatch", Qt::QueuedConnection, Q_ARG(QStringList, ids)); } else { status_ = QStringLiteral("Storage error: worker unavailable"); emit statusChanged(); } }
void StorageRecoveryBridge::discardSelectedRun() { selectedRunId_ = selectedRunId_.trimmed(); if (selectedRunId_.isEmpty()) { status_ = QStringLiteral("Discard rejected: no persisted run selected"); emit statusChanged(); return; } if (worker_) { if (!analysisResult_.isEmpty()) { analysisResult_.clear(); emit analysisResultChanged(); } if (analysisFrames_ != 0) { analysisFrames_ = 0; emit analysisProgressChanged(); } if (analysisRetryable_) { analysisRetryable_ = false; emit analysisRetryableChanged(); } if (!analysisError_.isEmpty()) { analysisError_.clear(); emit analysisErrorChanged(); } status_ = QStringLiteral("Discarding persisted recovery"); emit statusChanged(); QMetaObject::invokeMethod(worker_, "discard", Qt::QueuedConnection, Q_ARG(QString, selectedRunId_)); } else { status_ = QStringLiteral("Storage error: worker unavailable"); emit statusChanged(); } }
void StorageRecoveryBridge::retrySelectedRun() { selectedRunId_ = selectedRunId_.trimmed(); if (selectedRunId_.isEmpty()) { status_ = QStringLiteral("Retry rejected: no persisted run selected"); emit statusChanged(); return; } if (worker_) { if (!analysisResult_.isEmpty()) { analysisResult_.clear(); emit analysisResultChanged(); } if (analysisFrames_ != 0) { analysisFrames_ = 0; emit analysisProgressChanged(); } if (analysisRetryable_) { analysisRetryable_ = false; emit analysisRetryableChanged(); } if (!analysisError_.isEmpty()) { analysisError_.clear(); emit analysisErrorChanged(); } status_ = QStringLiteral("Retrying persisted recovery"); emit statusChanged(); QMetaObject::invokeMethod(worker_, "retry", Qt::QueuedConnection, Q_ARG(QString, selectedRunId_)); } else { status_ = QStringLiteral("Storage error: worker unavailable"); emit statusChanged(); } }
void StorageRecoveryBridge::findCached(QString content, QString engine, QString configuration, QString mode) {
  if (content.trimmed().isEmpty()) {
    status_ = QStringLiteral("Cache lookup rejected: content identity is required");
    emit statusChanged();
    return;
  }
  if (engine.trimmed().isEmpty() || configuration.trimmed().isEmpty() || mode.trimmed().isEmpty()) {
    status_ = QStringLiteral("Cache lookup rejected: complete cache identity is required");
    emit statusChanged();
    return;
  }
  if (!storageOpen_) {
    status_ = QStringLiteral("Cache lookup rejected: storage is not open");
    emit statusChanged();
    return;
  }
  if (worker_) {
    QMetaObject::invokeMethod(worker_, "findCached", Qt::QueuedConnection,
                              Q_ARG(QString, content), Q_ARG(QString, engine),
                              Q_ARG(QString, configuration), Q_ARG(QString, mode));
  } else {
    status_ = QStringLiteral("Storage error: worker unavailable");
    emit statusChanged();
  }
}

// This translation unit defines the private Worker QObject inline; include
// the generated meta-object implementation so Qt's automoc build is valid on
// all generators and platforms.
#include "storage_recovery_bridge.moc"
void StorageRecoveryBridge::invalidateCached(QString content, QString engine, QString configuration, QString mode) {
  if (content.trimmed().isEmpty()) {
    status_ = QStringLiteral("Cache invalidation rejected: content identity is required");
    emit statusChanged();
    return;
  }
  if (engine.trimmed().isEmpty() || configuration.trimmed().isEmpty() || mode.trimmed().isEmpty()) {
    status_ = QStringLiteral("Cache invalidation rejected: complete cache identity is required");
    emit statusChanged();
    return;
  }
  if (!storageOpen_) {
    status_ = QStringLiteral("Cache invalidation rejected: storage is not open");
    emit statusChanged();
    return;
  }
  if (worker_) {
    QMetaObject::invokeMethod(worker_, "invalidateCached", Qt::QueuedConnection,
                              Q_ARG(QString, content), Q_ARG(QString, engine),
                              Q_ARG(QString, configuration), Q_ARG(QString, mode));
  } else {
    status_ = QStringLiteral("Storage error: worker unavailable");
    emit statusChanged();
  }
}
void StorageRecoveryBridge::submitAnalysis(QString runId, QString path, QString contentHash, QString engine, QString configuration, QString mode) {
  if (runId.trimmed().isEmpty()) { status_ = QStringLiteral("Analysis rejected: run identity is required"); emit statusChanged(); return; }
  if (path.trimmed().isEmpty()) { status_ = QStringLiteral("Analysis rejected: source path is required"); emit statusChanged(); return; }
  if (contentHash.trimmed().isEmpty()) { status_ = QStringLiteral("Analysis rejected: content identity is required"); emit statusChanged(); return; }
  if (engine.trimmed().isEmpty() || configuration.trimmed().isEmpty() || mode.trimmed().isEmpty()) { status_ = QStringLiteral("Analysis rejected: complete analysis identity is required"); emit statusChanged(); return; }
  if (!storageOpen_) { status_ = QStringLiteral("Analysis rejected: storage is not open"); emit statusChanged(); return; }
  if (worker_) QMetaObject::invokeMethod(worker_, "submitAnalysis", Qt::QueuedConnection, Q_ARG(QString, runId), Q_ARG(QString, path), Q_ARG(QString, contentHash), Q_ARG(QString, engine), Q_ARG(QString, configuration), Q_ARG(QString, mode)); else { status_ = QStringLiteral("Storage error: worker unavailable"); emit statusChanged(); }
}
void StorageRecoveryBridge::reanalyze(QString runId, QString path, QString contentHash, QString engine, QString configuration, QString mode) {
  if (runId.trimmed().isEmpty() || path.trimmed().isEmpty() || contentHash.trimmed().isEmpty() || engine.trimmed().isEmpty() || configuration.trimmed().isEmpty() || mode.trimmed().isEmpty()) {
    status_ = QStringLiteral("Reanalysis rejected: complete source identity is required");
    emit statusChanged();
    return;
  }
  if (!storageOpen_) {
    status_ = QStringLiteral("Reanalysis rejected: storage is not open");
    emit statusChanged();
    return;
  }
  invalidateCached(contentHash, engine, configuration, mode);
  submitAnalysis(std::move(runId), std::move(path), std::move(contentHash), std::move(engine), std::move(configuration), std::move(mode));
}
void StorageRecoveryBridge::cancelAnalysis() { if (!storageOpen_) { status_ = QStringLiteral("Analysis cancellation rejected: storage is not open"); emit statusChanged(); return; } if (worker_) QMetaObject::invokeMethod(worker_, "cancelAnalysis", Qt::QueuedConnection); else { status_ = QStringLiteral("Storage error: worker unavailable"); emit statusChanged(); } }
void StorageRecoveryBridge::retryAnalysis() { if (!storageOpen_) { status_ = QStringLiteral("Analysis retry rejected: storage is not open"); emit statusChanged(); return; } if (worker_) QMetaObject::invokeMethod(worker_, "retryAnalysis", Qt::QueuedConnection); else { status_ = QStringLiteral("Storage error: worker unavailable"); emit statusChanged(); } }
void StorageRecoveryBridge::admitAnalysis(QString runId, QString content, QString engine, QString configuration, QString mode) {
  if (runId.trimmed().isEmpty() || content.trimmed().isEmpty() || engine.trimmed().isEmpty() || configuration.trimmed().isEmpty() || mode.trimmed().isEmpty()) {
    status_ = QStringLiteral("Analysis admission rejected: complete analysis identity is required");
    emit statusChanged();
    return;
  }
  if (!storageOpen_) {
    status_ = QStringLiteral("Analysis admission rejected: storage is not open");
    emit statusChanged();
    return;
  }
  if (worker_) {
    QMetaObject::invokeMethod(worker_, "admitAnalysis", Qt::QueuedConnection,
                              Q_ARG(QString, runId), Q_ARG(QString, content),
                              Q_ARG(QString, engine), Q_ARG(QString, configuration),
                              Q_ARG(QString, mode));
  } else {
    status_ = QStringLiteral("Storage error: worker unavailable");
    emit statusChanged();
  }
}
