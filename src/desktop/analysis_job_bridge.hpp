#pragma once
#include "beatglass/library/analysis_runner.hpp"
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <vector>

/** GUI-thread bridge for one source-attributed analysis job.
 * The operation is retained for explicit retry; cancellation is cooperative.
 */
class AnalysisJobBridge final : public QObject {
  Q_OBJECT
  Q_PROPERTY(qulonglong frames READ frames NOTIFY progressChanged)
  Q_PROPERTY(QString status READ status NOTIFY statusChanged)
  Q_PROPERTY(QString result READ result NOTIFY resultChanged)
  Q_PROPERTY(bool canCancel READ canCancel NOTIFY statusChanged)
  Q_PROPERTY(bool canRetry READ canRetry NOTIFY statusChanged)
  Q_PROPERTY(int batchAcceptedCount READ batchAcceptedCount NOTIFY batchChanged)
  Q_PROPERTY(int batchRejectedCount READ batchRejectedCount NOTIFY batchChanged)
  Q_PROPERTY(int batchCapacity READ batchCapacity CONSTANT)
public:
  explicit AnalysisJobBridge(beatglass::library_detail::JobExecutor&, QObject* parent = nullptr);
  std::shared_ptr<beatglass::library::RetryableAnalysis> submit(beatglass::library::AnalysisOperation);
  qulonglong frames() const noexcept { return frames_; }
  QString status() const { return status_; }
  QString result() const { return result_; }
  bool canCancel() const noexcept;
  bool canRetry() const noexcept;
  Q_INVOKABLE void cancel();
  Q_INVOKABLE void retry();
  /** Submit a bounded batch and retain each accepted operation for retry. */
  int submitBatch(std::vector<beatglass::library::AnalysisOperation> operations, int priority = 0);
  Q_INVOKABLE void cancelBatch();
  Q_INVOKABLE void retryBatch();
  int batchAcceptedCount() const noexcept { return batchAccepted_; }
  int batchRejectedCount() const noexcept { return batchRejected_; }
  int batchCapacity() const noexcept { return batchCapacity_; }
signals:
  void progressChanged();
  void statusChanged();
  void resultChanged();
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
  const int batchCapacity_{2};
  QTimer poll_timer_;
  QPointer<AnalysisJobBridge> weak_self_{this};
  qulonglong frames_{};
  QString status_{QStringLiteral("Idle")};
  QString result_;
};
