#include "storage_recovery_bridge.hpp"
#include "beatglass/library/analysis_coordinator.hpp"
#include "beatglass/library/file_analysis_operation.hpp"
#include <QMetaObject>

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
    auto handle = coordinator_.submit(runId.toStdString(), std::move(operation));
    emit analysis(handle ? QStringLiteral("Analysis queued") : QStringLiteral("Analysis rejected"));
  }
signals:
  void result(bool ok, QString status);
  void runsChanged(QStringList ids);
  void cached(bool hit, QString result);
  void analysis(QString status);
private:
  beatglass::library::AnalysisCoordinator coordinator_;
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
