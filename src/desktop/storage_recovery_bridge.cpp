#include "storage_recovery_bridge.hpp"
#include "beatglass/library/analysis_coordinator.hpp"
#include "beatglass/library/file_analysis_operation.hpp"
#include <QMetaObject>
#include <QTimer>

class StorageRecoveryBridge::Worker final : public QObject {
  Q_OBJECT
public slots:
  void open(QString path) {
    beatglass::library::StorageConfig config{path.toStdString()};
    std::string diagnostic;
    const bool ok = coordinator_.open_storage(config, &diagnostic);
    emit result(ok, QString::fromStdString(ok ? "Storage open" : diagnostic));
    if (ok) refresh();
  }
  void close() { coordinator_.close_storage(); emit result(true, QStringLiteral("Storage closed")); }
  void refresh() {
    const auto runs = coordinator_.storage().retryable_runs();
    QStringList ids;
    for (const auto& run : runs) ids.push_back(QString::fromStdString(run.id));
    emit runsChanged(ids);
  }
  void discard(QString id) { const bool ok = coordinator_.storage().discard_retryable(id.toStdString()); emit result(ok, ok ? QStringLiteral("Retryable run discarded") : QStringLiteral("Retryable run not found")); if (ok) refresh(); }
  void findCached(QString content, QString engine, QString configuration, QString mode) {
    beatglass::AnalysisIdentity identity{content.toStdString(), engine.toStdString(), configuration.toStdString(), mode.toStdString()};
    std::string result; emit cached(coordinator_.storage().find_cached(identity, result), QString::fromStdString(result));
  }
  void submitAnalysis(QString runId, QString path, QString contentHash, QString engine, QString configuration, QString mode) {
    beatglass::library::AnalysisIdentity identity{contentHash.toStdString(), engine.toStdString(), configuration.toStdString(), mode.toStdString()};
    auto operation = beatglass::library::make_file_decode_operation(path.toStdString(), std::move(identity));
    operation.publish = [this](const std::string& value) { emit analysisResult(QString::fromStdString(value)); };
    auto handle = coordinator_.submit(runId.toStdString(), std::move(operation));
    handle_ = std::move(handle); emit analysis(handle_ ? QStringLiteral("Analysis queued") : QStringLiteral("Analysis rejected"));
    if (handle_) QTimer::singleShot(50, this, &Worker::pollAnalysis);
  }
<<<<<<< HEAD
  void cancelAnalysis() { if (handle_) { handle_->cancel(); pollAnalysis(); } }
  void retryAnalysis() { if (handle_ && handle_->retry()) { emit analysis(QStringLiteral("Analysis queued")); QTimer::singleShot(50, this, &Worker::pollAnalysis); } }
=======
  void admitAnalysis(QString runId, QString content, QString engine, QString configuration, QString mode) {
    beatglass::AnalysisIdentity identity{content.toStdString(), engine.toStdString(), configuration.toStdString(), mode.toStdString()};
    const auto admission = coordinator_.storage().admit_analysis(runId.toStdString(), identity);
    emit admitted(admission.analysis_reserved, QString::fromStdString(admission.cached.value_or(std::string{})));
  }
>>>>>>> 17d8535a (feat: expose analysis admission in desktop bridge)
signals:
  void result(bool ok, QString status);
  void runsChanged(QStringList ids);
  void cached(bool hit, QString result);
  void analysis(QString status);
<<<<<<< HEAD
  void analysisResult(QString result);
=======
  void admitted(bool reserved, QString cachedResult);
>>>>>>> 17d8535a (feat: expose analysis admission in desktop bridge)
private:
  void pollAnalysis() {
    if (!handle_) return;
    using beatglass::library_detail::TrackedState;
    const auto state = handle_->state();
    const auto status = state == TrackedState::queued ? "Analysis queued" : state == TrackedState::running ? "Analysis running" : state == TrackedState::completed ? "Analysis complete" : state == TrackedState::cancelled ? "Analysis cancelled" : "Analysis failed";
    emit analysis(QString::fromLatin1(status));
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
    emit statusChanged();
  }, Qt::QueuedConnection);
  connect(worker_, &Worker::runsChanged, this, [this](const QStringList& ids) {
    retryableRunIds_ = ids; emit retryableRunsChanged();
  }, Qt::QueuedConnection);
  connect(worker_, &Worker::cached, this, [this](bool hit, const QString& result) {
    cacheHit_ = hit; cachedResult_ = result; emit cachedResultChanged();
  }, Qt::QueuedConnection);
  connect(worker_, &Worker::analysis, this, [this](const QString& text) { analysisStatus_ = text; emit analysisStatusChanged(); }, Qt::QueuedConnection);
<<<<<<< HEAD
  connect(worker_, &Worker::analysisResult, this, [this](const QString& text) { analysisResult_ = text; emit analysisResultChanged(); }, Qt::QueuedConnection);
=======
  connect(worker_, &Worker::admitted, this, [this](bool reserved, const QString& result) { analysisReserved_ = reserved; analysisAdmissionResult_ = result; emit analysisAdmissionChanged(); }, Qt::QueuedConnection);
>>>>>>> 17d8535a (feat: expose analysis admission in desktop bridge)
  workerThread_.start();
}

StorageRecoveryBridge::~StorageRecoveryBridge() { close(); workerThread_.quit(); workerThread_.wait(); }
void StorageRecoveryBridge::setDatabasePath(const QString& path) { if (databasePath_ == path) return; databasePath_ = path; emit databasePathChanged(); }
void StorageRecoveryBridge::open() {
  if (databasePath_.isEmpty()) { status_ = QStringLiteral("Storage error: database path is required"); emit statusChanged(); return; }
  QMetaObject::invokeMethod(worker_, "open", Qt::QueuedConnection, Q_ARG(QString, databasePath_));
  status_ = QStringLiteral("Opening storage"); emit statusChanged();
}
void StorageRecoveryBridge::close() { if (worker_) QMetaObject::invokeMethod(worker_, "close", Qt::QueuedConnection); }
void StorageRecoveryBridge::refreshRetryableRuns() { if (worker_) QMetaObject::invokeMethod(worker_, "refresh", Qt::QueuedConnection); }
void StorageRecoveryBridge::selectRun(const QString& id) { if (selectedRunId_ == id) return; selectedRunId_ = id; emit selectedRunChanged(); }
void StorageRecoveryBridge::discardSelectedRun() { if (!selectedRunId_.isEmpty()) QMetaObject::invokeMethod(worker_, "discard", Qt::QueuedConnection, Q_ARG(QString, selectedRunId_)); }
void StorageRecoveryBridge::findCached(QString content, QString engine, QString configuration, QString mode) {
  if (worker_) QMetaObject::invokeMethod(worker_, "findCached", Qt::QueuedConnection, Q_ARG(QString, content), Q_ARG(QString, engine), Q_ARG(QString, configuration), Q_ARG(QString, mode));
}
void StorageRecoveryBridge::submitAnalysis(QString runId, QString path, QString contentHash, QString engine, QString configuration, QString mode) {
  if (worker_) QMetaObject::invokeMethod(worker_, "submitAnalysis", Qt::QueuedConnection, Q_ARG(QString, runId), Q_ARG(QString, path), Q_ARG(QString, contentHash), Q_ARG(QString, engine), Q_ARG(QString, configuration), Q_ARG(QString, mode));
}
<<<<<<< HEAD
void StorageRecoveryBridge::cancelAnalysis() { if (worker_) QMetaObject::invokeMethod(worker_, "cancelAnalysis", Qt::QueuedConnection); }
void StorageRecoveryBridge::retryAnalysis() { if (worker_) QMetaObject::invokeMethod(worker_, "retryAnalysis", Qt::QueuedConnection); }
=======
void StorageRecoveryBridge::admitAnalysis(QString runId, QString content, QString engine, QString configuration, QString mode) {
  if (worker_) QMetaObject::invokeMethod(worker_, "admitAnalysis", Qt::QueuedConnection, Q_ARG(QString, runId), Q_ARG(QString, content), Q_ARG(QString, engine), Q_ARG(QString, configuration), Q_ARG(QString, mode));
}
>>>>>>> 17d8535a (feat: expose analysis admission in desktop bridge)
