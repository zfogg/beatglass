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
  Q_PROPERTY(bool cacheHit READ cacheHit NOTIFY cachedResultChanged)
  Q_PROPERTY(QString cachedResult READ cachedResult NOTIFY cachedResultChanged)
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
  /** Look up a completed result on the storage worker without starting analysis. */
  Q_INVOKABLE void findCached(QString content, QString engine, QString configuration, QString mode);
  bool cacheHit() const { return cacheHit_; }
  QString cachedResult() const { return cachedResult_; }
public slots:
  void setDatabasePath(const QString& path);
  void selectRun(const QString& id);
signals:
  void databasePathChanged();
  void statusChanged();
  void retryableRunsChanged();
  void selectedRunChanged();
  void cachedResultChanged();
private:
  class Worker;
  QThread workerThread_;
  Worker* worker_{};
  QString databasePath_;
  QString status_{QStringLiteral("Storage not configured")};
  QStringList retryableRunIds_;
  QString selectedRunId_;
  bool cacheHit_{};
  QString cachedResult_;
};
