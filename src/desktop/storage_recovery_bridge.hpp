#pragma once
#include <QObject>
#include <QString>
#include <QThread>

/** GUI bridge for explicitly configured, worker-owned library storage.
 * No database path is inferred; callers must inject one before opening.
 */
class StorageRecoveryBridge final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString databasePath READ databasePath WRITE setDatabasePath NOTIFY databasePathChanged)
  Q_PROPERTY(QString status READ status NOTIFY statusChanged)
public:
  explicit StorageRecoveryBridge(QObject* parent = nullptr);
  ~StorageRecoveryBridge() override;
  QString databasePath() const { return databasePath_; }
  QString status() const { return status_; }
  Q_INVOKABLE void open();
  Q_INVOKABLE void close();
public slots:
  void setDatabasePath(const QString& path);
signals:
  void databasePathChanged();
  void statusChanged();
private:
  class Worker;
  QThread workerThread_;
  Worker* worker_{};
  QString databasePath_;
  QString status_{QStringLiteral("Storage not configured")};
};
