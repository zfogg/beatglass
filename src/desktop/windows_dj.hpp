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
class QTimer;

/** Windows-only GUI-thread bridge to a worker-owned experimental tempo source. The worker coalesces updates for nominal 50 ms GUI polling; invalidation is visible after polling, while stop clears immediately and rejects older generations. Destruction joins the worker. It does not provide key, chords, beat phase or default-device following. */
class WindowsDj : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantList sources READ sources NOTIFY changed)
  Q_PROPERTY(QString status READ status NOTIFY changed)
  Q_PROPERTY(QString tempo READ tempo NOTIFY changed)
  Q_PROPERTY(bool active READ active NOTIFY changed)
public:
  /** Construct on the GUI thread and start a control worker; parent shares that thread. */
  explicit WindowsDj(QObject* parent = nullptr);
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
  /** Start monitoring an opaque endpoint ID; empty IDs are ignored. */
  Q_INVOKABLE void start(const QString& endpoint);
  /** Invalidate the visible result and request stop. */
  Q_INVOKABLE void stop();
  /** Ask the worker to enumerate output endpoints again. */
  Q_INVOKABLE void refresh();
signals:
  /** Emitted on the GUI thread after visible state changes. */
  void changed();
private:
  struct Value { std::uint64_t generation{}; QString status; std::optional<float> bpm; bool active{}; QVariantList sources; };
  void run();
  void drain();
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
