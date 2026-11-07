#pragma once
#include <QObject>
#include <QString>
#include <QVariantList>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>
#include "beatglass/capture/source_lifecycle.hpp"
class QTimer;

/** Windows-only GUI-thread bridge to a worker-owned experimental tempo source. The worker coalesces updates for nominal 50 ms GUI polling; invalidation is visible after polling, while stop clears immediately and rejects older generations. Destruction joins the worker. It supports the Windows default follower but does not provide key, chords or beat phase. */
class WindowsDj : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantList sources READ sources NOTIFY sourcesChanged)
  Q_PROPERTY(QString status READ status NOTIFY changed)
  Q_PROPERTY(QString tempo READ tempo NOTIFY changed)
  Q_PROPERTY(bool active READ active NOTIFY changed)
public:
  /** Construct on the GUI thread with the process-owned epoch allocator, which must outlive this object and its joined worker. Allocation never occurs on the capture callback. */
  explicit WindowsDj(beatglass::SourceEpochAllocator& epochs, QObject* parent = nullptr);
  // Documented manually in the API index for Hawkmoth destructor parsing.
  ~WindowsDj() override;
  /** Snapshot of discovered endpoint names and opaque IDs. */
  QVariantList sources() const { return sources_; }
  /** Current user-facing control or capture status. */
  QString status() const { return status_; }
  /** Current BPM or an em dash when unavailable. */
  QString tempo() const { return tempo_; }
  /** Whether capture has been requested and not stopped or failed. */
  bool active() const { return active_; }
  /** Start monitoring an opaque endpoint ID or the reserved __beatglass_default__ selection; empty IDs are ignored. */
  Q_INVOKABLE void start(const QString& endpoint);
  /** Invalidate the visible result and request stop. */
  Q_INVOKABLE void stop();
  /** Ask the worker to enumerate output endpoints again. */
  Q_INVOKABLE void refresh();
signals:
  /** Emitted on the GUI thread only when the endpoint list changes. */
  void sourcesChanged();
  /** Emitted on the GUI thread after status, tempo or active changes. */
  void changed();
private:
  struct Value { std::uint64_t generation{}; QString status; std::optional<float> bpm; bool active{}; QVariantList sources; };
  void run();
  void drain();
  beatglass::SourceEpochAllocator& epochs_;
  std::mutex mutex_;
  std::condition_variable wake_;
  bool quitting_{}, requested_{}, refresh_{true}, pending_{};
  std::uint64_t generation_{};
  std::string endpoint_;
  Value latest_;
  std::thread worker_;
  QTimer* timer_{};
  QVariantList sources_;
  QString status_ = QStringLiteral("Stopped"), tempo_ = QStringLiteral("\u2014");
  bool active_{};
};
