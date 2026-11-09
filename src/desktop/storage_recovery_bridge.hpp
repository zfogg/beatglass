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
  Q_PROPERTY(QString analysisError READ analysisError NOTIFY analysisErrorChanged)
  Q_PROPERTY(bool analysisRetryable READ analysisRetryable NOTIFY analysisRetryableChanged)
  Q_PROPERTY(qulonglong analysisFrames READ analysisFrames NOTIFY analysisProgressChanged)
  Q_PROPERTY(QString analysisResult READ analysisResult NOTIFY analysisResultChanged)
  Q_PROPERTY(bool analysisReserved READ analysisReserved NOTIFY analysisAdmissionChanged)
  Q_PROPERTY(QString analysisAdmissionResult READ analysisAdmissionResult NOTIFY analysisAdmissionChanged)
  Q_PROPERTY(QString resetStatus READ resetStatus NOTIFY resetStatusChanged)
public:
  explicit StorageRecoveryBridge(QObject* parent = nullptr);
  ~StorageRecoveryBridge() override;
  QString databasePath() const { return databasePath_; }
  QString status() const { return status_; }
  QStringList retryableRunIds() const { return retryableRunIds_; }
  QStringList retryableRunDetails() const { return retryableRunDetails_; }
  QString selectedRunId() const { return selectedRunId_; }
  /** Queue storage open on the dedicated worker; returns before SQLite work begins. */
  Q_INVOKABLE void open();
  /** Queue storage close on the dedicated worker and clear GUI-owned state immediately. */
  Q_INVOKABLE void close();
  /** Queue a durable retryable-run refresh; results are delivered via retryableRunsChanged. */
  Q_INVOKABLE void refreshRetryableRuns();
  /** Discard only terminal persisted recovery; active jobs retain selection and report rejection. */
  Q_INVOKABLE void discardSelectedRun();
  /** Queue an atomic batch discard; source audio is never modified. */
  Q_INVOKABLE void discardRuns(const QStringList& ids);
  /** Retry the selected persisted run using its stored source and decoder snapshot. */
  Q_INVOKABLE void retrySelectedRun();
  /** Look up a completed result on the storage worker without starting analysis. */
  Q_INVOKABLE void findCached(QString content, QString engine, QString configuration, QString mode);
  /** Remove one completed cache identity without touching source files or retry records. */
  Q_INVOKABLE void invalidateCached(QString content, QString engine, QString configuration, QString mode);
  /** Submit a path-backed analysis through the worker-owned coordinator. */
  Q_INVOKABLE void submitAnalysis(QString runId, QString path, QString contentHash, QString engine, QString configuration, QString mode);
  /** Invalidate the selected identity and immediately submit a fresh worker-owned analysis. */
  Q_INVOKABLE void reanalyze(QString runId, QString path, QString contentHash, QString engine, QString configuration, QString mode);
  /** Request cancellation of the worker-owned active analysis. */
  Q_INVOKABLE void cancelAnalysis();
  /** Retry the retained active analysis handle after cancellation or failure. */
  Q_INVOKABLE void retryAnalysis();
  bool cacheHit() const { return cacheHit_; }
  QString cachedResult() const { return cachedResult_; }
  QString analysisStatus() const { return analysisStatus_; }
  QString analysisError() const { return analysisError_; }
  bool analysisRetryable() const { return analysisRetryable_; }
  qulonglong analysisFrames() const { return analysisFrames_; }
  QString analysisResult() const { return analysisResult_; }
  Q_INVOKABLE void admitAnalysis(QString runId, QString content, QString engine, QString configuration, QString mode);
  bool analysisReserved() const { return analysisReserved_; }
  QString analysisAdmissionResult() const { return analysisAdmissionResult_; }
  QString resetStatus() const { return resetStatus_; }
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
  void analysisErrorChanged();
  void analysisRetryableChanged();
  void analysisProgressChanged();
  void analysisResultChanged();
  void analysisAdmissionChanged();
  void resetStatusChanged();
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
  QString analysisError_;
  bool analysisRetryable_{};
  qulonglong analysisFrames_{};
  QString analysisResult_;
  bool analysisReserved_{};
  QString analysisAdmissionResult_;
  QString resetStatus_;
};
