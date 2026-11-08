#include "storage_recovery_bridge.hpp"
#include "beatglass/library/storage_owner.hpp"
#include <QMetaObject>

class StorageRecoveryBridge::Worker final : public QObject {
  Q_OBJECT
public slots:
  void open(QString path) {
    beatglass::library::StorageConfig config{path.toStdString()};
    std::string diagnostic;
    const bool ok = owner_.open(config, &diagnostic);
    emit result(ok, QString::fromStdString(ok ? "Storage open" : diagnostic));
  }
  void close() { owner_.close(); emit result(true, QStringLiteral("Storage closed")); }
signals:
  void result(bool ok, QString status);
private:
  beatglass::library::StorageOwner owner_;
};

StorageRecoveryBridge::StorageRecoveryBridge(QObject* parent) : QObject(parent), worker_(new Worker) {
  worker_->moveToThread(&workerThread_);
  connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
  connect(worker_, &Worker::result, this, [this](bool ok, const QString& text) {
    status_ = ok ? text : QStringLiteral("Storage error: ") + text;
    emit statusChanged();
  }, Qt::QueuedConnection);
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
