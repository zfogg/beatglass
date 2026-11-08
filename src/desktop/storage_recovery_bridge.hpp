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
  Q_PROPERTY(QStringList retryableRunDetails READ retryableRunDetails NOTIFY retryableRunsChanged)
  Q_PROPERTY(QString selectedRunId READ selectedRunId WRITE selectRun NOTIFY selectedRunChanged)
  Q_PROPERTY(bool cacheHit READ cacheHit NOTIFY cachedResultChanged)
  Q_PROPERTY(QString cachedResult READ cachedResult NOTIFY cachedResultChanged)
  Q_PROPERTY(QString analysisStatus READ analysisStatus NOTIFY analysisStatusChanged)
  Q_PROPERTY(QString analysisResult READ analysisResult NOTIFY analysisResultChanged)
  Q_PROPERTY(bool analysisReserved READ analysisReserved NOTIFY analysisAdmissionChanged)
  Q_PROPERTY(QString analysisAdmissionResult READ analysisAdmissionResult NOTIFY analysisAdmissionChanged)
public:
  explicit StorageRecoveryBridge(QObject* parent = nullptr);
  ~StorageRecoveryBridge() override;
  QString databasePath() const { return databasePath_; }
  QString status() const { return status_; }
  QStringList retryableRunIds() const { return retryableRunIds_; }
  QStringList retryableRunDetails() const { return retryableRunDetails_; }
  QString selectedRunId() const { return selectedRunId_; }
  Q_INVOKABLE void open();
  Q_INVOKABLE void close();
  Q_INVOKABLE void refreshRetryableRuns();
  Q_INVOKABLE void discardSelectedRun();
  /** Retry the selected persisted run using its stored source and decoder snapshot. */
  Q_INVOKABLE void retrySelectedRun();
  /** Look up a completed result on the storage worker without starting analysis. */
  Q_INVOKABLE void findCached(QString content, QString engine, QString configuration, QString mode);
  /** Remove one completed cache identity without touching source files or retry records. */
  Q_INVOKABLE void invalidateCached(QString content, QString engine, QString configuration, QString mode);
  /** Submit a path-backed analysis through the worker-owned coordinator. */
  Q_INVOKABLE void submitAnalysis(QString runId, QString path, QString contentHash, QString engine, QString configuration, QString mode);
  Q_INVOKABLE void cancelAnalysis();
  Q_INVOKABLE void retryAnalysis();
  bool cacheHit() const { return cacheHit_; }
  QString cachedResult() const { return cachedResult_; }
  QString analysisStatus() const { return analysisStatus_; }
  QString analysisResult() const { return analysisResult_; }
  Q_INVOKABLE void admitAnalysis(QString runId, QString content, QString engine, QString configuration, QString mode);
  bool cacheHit() const { return cacheHit_; }
  QString cachedResult() const { return cachedResult_; }
  QString analysisStatus() const { return analysisStatus_; }
  bool analysisReserved() const { return analysisReserved_; }
  QString analysisAdmissionResult() const { return analysisAdmissionResult_; }
public slots:
  void setDatabasePath(const QString& path);
  void selectRun(const QString& id);
signals:
  void databasePathChanged();
  void statusChanged();
  void retryableRunsChanged();
  void selectedRunChanged();
  void cachedResultChanged();
  void analysisStatusChanged();
  void analysisResultChanged();
  void analysisAdmissionChanged();
private:
  class Worker;
  QThread workerThread_;
  Worker* worker_{};
  QString databasePath_;
  QString status_{QStringLiteral("Storage not configured")};
  QStringList retryableRunIds_;
  QStringList retryableRunDetails_;
  QString selectedRunId_;
  bool cacheHit_{};
  QString cachedResult_;
  QString analysisStatus_{QStringLiteral("Analysis idle")};
  QString analysisResult_;
  bool analysisReserved_{};
  QString analysisAdmissionResult_;
};
