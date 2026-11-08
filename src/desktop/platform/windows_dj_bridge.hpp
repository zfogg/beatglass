#pragma once
#include "windows_default_tempo.hpp"
#include <QObject>
#include <QTimer>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>

Q_DECLARE_METATYPE(beatglass::PresentationUpdate)

namespace beatglass::windows {
/** Provisional GUI-thread bridge for the default-output tempo worker. It does not create controls or an overlay. */
class DjBridge final : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool active READ active NOTIFY stateChanged)
  Q_PROPERTY(QString status READ status NOTIFY stateChanged)
  Q_PROPERTY(QString selectedEndpoint READ selectedEndpoint NOTIFY stateChanged)
public:
  /** Construct on the GUI thread with the process-session allocator. The caller retains ownership and must keep it alive until after this bridge is destroyed and its worker has joined. Issuance happens on the worker, never the capture callback. The optional discovery function is a test seam. */
  explicit DjBridge(SourceEpochAllocator& epochs, QObject* parent = nullptr, std::function<EndpointResult()> discovery = {});
  // Documented manually in the API reference due to Hawkmoth destructor parsing.
  ~DjBridge() override;
  /** Request default-output capture; it starts asynchronously on the owned worker. */
  Q_INVOKABLE void start();
  /** Request stop and a revisioned clear; delivery is asynchronous. A later start cannot bypass that stop even when both are requested before the worker wakes. Capture start is serialized with this request; stop may wait for the short start request, which does not wait for WASAPI startup. */
  Q_INVOKABLE void stop();
  Q_INVOKABLE void selectEndpoint(const QString& id);
  /** Whether capture is requested. Read on the GUI thread. */
  bool active() const;
  /** Most recently delivered status. Read on the GUI thread. */
  QString status() const;
  QString selectedEndpoint() const;
  /** Emitted on the GUI thread with a revisioned snapshot or clear. The receiver must apply it on that thread and handle its own lifetime. */
  Q_SIGNAL void presentationUpdate(beatglass::PresentationUpdate update);
  /** Emitted on the GUI thread when request or delivered status changes. */
  Q_SIGNAL void stateChanged();
private:
  void run(std::function<EndpointResult()> discovery);
  void drain();
  SourceEpochAllocator& epochs_;
  mutable std::mutex mutex_;
  std::condition_variable wake_;
  bool requested_{}, quitting_{}, reset_pending_{}, commands_exhausted_{};
  std::optional<std::string> pending_endpoint_;
  QString selected_endpoint_;
  std::uint64_t request_generation_{};
  std::optional<PresentationUpdate> pending_;
  std::string pending_status_;
  bool have_status_{};
  QString status_{QStringLiteral("Stopped")};
  QTimer timer_;
  std::thread worker_;
};
}
