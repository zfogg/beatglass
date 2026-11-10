#pragma once
#include "beatglass/library/analysis_runner.hpp"
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QStringList>
#include <vector>

/** GUI-thread bridge for one source-attributed analysis job.
 * The operation is retained for explicit retry; cancellation is cooperative.
 */
class AnalysisJobBridge final : public QObject {
  Q_OBJECT
  Q_PROPERTY(qulonglong frames READ frames NOTIFY progressChanged)
  Q_PROPERTY(qulonglong totalFrames READ totalFrames NOTIFY progressChanged)
  Q_PROPERTY(bool progressKnown READ progressKnown NOTIFY progressChanged)
  Q_PROPERTY(double progressFraction READ progressFraction NOTIFY progressChanged)
  Q_PROPERTY(QString status READ status NOTIFY statusChanged)
  Q_PROPERTY(QString result READ result NOTIFY resultChanged)
  Q_PROPERTY(QString sourcePath READ sourcePath NOTIFY sourceChanged)
  Q_PROPERTY(QString failureReason READ failureReason NOTIFY failureChanged)
  Q_PROPERTY(QString recoveryStatus READ recoveryStatus NOTIFY statusChanged)
  Q_PROPERTY(QString engineVersion READ engineVersion NOTIFY provenanceChanged)
  Q_PROPERTY(QString configurationId READ configurationId NOTIFY provenanceChanged)
  Q_PROPERTY(QString analysisMode READ analysisMode NOTIFY provenanceChanged)
  Q_PROPERTY(bool canCancel READ canCancel NOTIFY statusChanged)
  Q_PROPERTY(bool canRetry READ canRetry NOTIFY statusChanged)
  Q_PROPERTY(int batchAcceptedCount READ batchAcceptedCount NOTIFY batchChanged)
  Q_PROPERTY(int batchRejectedCount READ batchRejectedCount NOTIFY batchChanged)
  Q_PROPERTY(int batchCapacity READ batchCapacity CONSTANT)
  Q_PROPERTY(int batchResultsPublished READ batchResultsPublished NOTIFY batchChanged)
  Q_PROPERTY(QString batchLastResult READ batchLastResult NOTIFY batchChanged)
  Q_PROPERTY(QString batchLastSource READ batchLastSource NOTIFY batchChanged)
  Q_PROPERTY(int batchFailedCount READ batchFailedCount NOTIFY batchChanged)
  Q_PROPERTY(int batchCancelledCount READ batchCancelledCount NOTIFY batchChanged)
  Q_PROPERTY(int batchCompletedCount READ batchCompletedCount NOTIFY batchChanged)
  Q_PROPERTY(int batchQueuedCount READ batchQueuedCount NOTIFY batchChanged)
public:
  explicit AnalysisJobBridge(beatglass::library_detail::JobExecutor&, QObject* parent = nullptr);
  std::shared_ptr<beatglass::library::RetryableAnalysis> submit(beatglass::library::AnalysisOperation);
  qulonglong frames() const noexcept { return frames_; }
  qulonglong totalFrames() const noexcept { return total_frames_; }
  bool progressKnown() const noexcept { return progress_known_; }
  double progressFraction() const noexcept {
    if (!progress_known_) return -1.0;
    if (total_frames_ == 0) return 1.0;
    return static_cast<double>(frames_) / static_cast<double>(total_frames_);
  }
  QString status() const { return status_; }
  QString result() const { return result_; }
  QString sourcePath() const { return sourcePath_; }
  QString failureReason() const { return failureReason_; }
  QString recoveryStatus() const { return recoveryStatus_; }
  QString engineVersion() const { return engineVersion_; }
  QString configurationId() const { return configurationId_; }
  QString analysisMode() const { return analysisMode_; }
  bool canCancel() const noexcept;
  bool canRetry() const noexcept;
  /** Cancel the active job, publish Cancelled immediately, and clear stale result presentation. */
  Q_INVOKABLE void cancel();
  Q_INVOKABLE void retry();
  /** Discard the terminal single-job recovery state without touching source files. */
  Q_INVOKABLE void discard();
  /** Submit a path-backed provisional decoder operation on the worker queue. */
  Q_INVOKABLE bool submitFile(const QString& path, const QString& contentHash,
                              const QString& engineVersion, const QString& configurationId,
                              const QString& mode = QStringLiteral("offline"));
  /** Fingerprint and queue one selected local file using the desktop defaults. */
  Q_INVOKABLE bool submitSelectedFile(const QString& path);
  /** Fingerprint and queue a bounded batch of selected local files. */
  Q_INVOKABLE int submitSelectedFiles(const QStringList& paths);
  /** Submit a bounded batch, publish Queued/Failed aggregate state, and retain accepted operations for retry. */
  int submitBatch(std::vector<beatglass::library::AnalysisOperation> operations, int priority = 0);
  /** Cancel active batch jobs, publish Cancelled immediately, and clear stale result/batch presentation. */
  Q_INVOKABLE void cancelBatch();
  /** Retry terminal batch jobs and immediately publish the refreshed aggregate state.
   *
   * Returns true only when at least one retained operation accepted the retry.
   */
  Q_INVOKABLE bool retryBatch();
  /** Discard terminal batch recovery state without touching source files. */
  Q_INVOKABLE void discardBatch();
  int batchAcceptedCount() const noexcept { return batchAccepted_; }
  int batchRejectedCount() const noexcept { return batchRejected_; }
  int batchCapacity() const noexcept { return batchCapacity_; }
  int batchResultsPublished() const noexcept { return batchResultsPublished_; }
  QString batchLastResult() const { return batchLastResult_; }
  QString batchLastSource() const { return batchLastSource_; }
  int batchFailedCount() const noexcept { return batchFailedCount_; }
  int batchCancelledCount() const noexcept { return batchCancelledCount_; }
  int batchCompletedCount() const noexcept { return batchCompletedCount_; }
  int batchQueuedCount() const noexcept { return batchAccepted_ - batchCompletedCount_ - batchFailedCount_ - batchCancelledCount_; }
signals:
  void progressChanged();
  void statusChanged();
  void resultChanged();
  void sourceChanged();
  void failureChanged();
  void provenanceChanged();
  void batchChanged();
private slots:
  void poll();
private:
  void setStatus();
  beatglass::library::AnalysisRunner runner_;
  std::shared_ptr<beatglass::library::RetryableAnalysis> retryable_;
  std::vector<std::shared_ptr<beatglass::library::RetryableAnalysis>> batch_;
  int batchAccepted_{};
  int batchRejected_{};
  int batchResultsPublished_{};
  QString batchLastResult_;
  QString batchLastSource_;
  int batchFailedCount_{};
  int batchCancelledCount_{};
  int batchCompletedCount_{};
  const int batchCapacity_{2};
  QTimer poll_timer_;
  QPointer<AnalysisJobBridge> weak_self_{this};
  qulonglong frames_{};
  qulonglong total_frames_{};
  bool progress_known_{};
  QString status_{QStringLiteral("Idle")};
  QString result_;
  QString sourcePath_;
  QString failureReason_;
  QString recoveryStatus_;
  QString engineVersion_;
  QString configurationId_;
  QString analysisMode_;
};
