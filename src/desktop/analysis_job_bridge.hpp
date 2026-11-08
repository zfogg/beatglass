#pragma once
#include "beatglass/library/analysis_runner.hpp"
#include <QObject>
#include <QTimer>

/** GUI-thread bridge for one source-attributed analysis job.
 * The operation is retained for explicit retry; cancellation is cooperative.
 */
class AnalysisJobBridge final : public QObject {
  Q_OBJECT
  Q_PROPERTY(qulonglong frames READ frames NOTIFY progressChanged)
  Q_PROPERTY(QString status READ status NOTIFY statusChanged)
  Q_PROPERTY(bool canCancel READ canCancel NOTIFY statusChanged)
  Q_PROPERTY(bool canRetry READ canRetry NOTIFY statusChanged)
public:
  explicit AnalysisJobBridge(beatglass::library_detail::JobExecutor&, QObject* parent = nullptr);
  std::shared_ptr<beatglass::library_detail::JobHandle> submit(beatglass::library::AnalysisOperation);
  qulonglong frames() const noexcept { return frames_; }
  QString status() const { return status_; }
  bool canCancel() const noexcept;
  bool canRetry() const noexcept;
  Q_INVOKABLE void cancel();
  Q_INVOKABLE void retry();
signals:
  void progressChanged();
  void statusChanged();
private slots:
  void poll();
private:
  void setStatus();
  beatglass::library::AnalysisOperation last_operation_;
  beatglass::library::AnalysisRunner runner_;
  std::shared_ptr<beatglass::library_detail::JobHandle> handle_;
  QTimer poll_timer_;
  qulonglong frames_{};
  QString status_{QStringLiteral("Idle")};
};
