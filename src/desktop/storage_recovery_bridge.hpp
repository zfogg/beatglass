#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>

/** GUI bridge for explicitly configured, worker-owned library storage.
 * No database path is inferred; callers must inject one before opening.
 */
class StorageRecoveryBridge final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString databasePath READ databasePath WRITE setDatabasePath NOTIFY databasePathChanged)
  Q_PROPERTY(QString status READ status NOTIFY statusChanged)
  Q_PROPERTY(QStringList retryableRunIds READ retryableRunIds NOTIFY retryableRunsChanged)
  Q_PROPERTY(QString selectedRunId READ selectedRunId WRITE selectRun NOTIFY selectedRunChanged)
public:
  explicit StorageRecoveryBridge(QObject* parent = nullptr);
  ~StorageRecoveryBridge() override;
  QString databasePath() const { return databasePath_; }
  QString status() const { return status_; }
  QStringList retryableRunIds() const { return retryableRunIds_; }
  QString selectedRunId() const { return selectedRunId_; }
  Q_INVOKABLE void open();
  Q_INVOKABLE void close();
  Q_INVOKABLE void refreshRetryableRuns();
  Q_INVOKABLE void discardSelectedRun();
public slots:
  void setDatabasePath(const QString& path);
  void selectRun(const QString& id);
signals:
  void databasePathChanged();
  void statusChanged();
  void retryableRunsChanged();
  void selectedRunChanged();
private:
  class Worker;
  QThread workerThread_;
  Worker* worker_{};
  QString databasePath_;
  QString status_{QStringLiteral("Storage not configured")};
  QStringList retryableRunIds_;
  QString selectedRunId_;
};
