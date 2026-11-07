#pragma once
#include <QObject>
#include <QLockFile>
#include <QLocalServer>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include <memory>

class QMenu;
class QMessageBox;
class QLocalSocket;
class QQuickWindow;
class QSystemTrayIcon;

/** GUI-thread owner of the provisional per-user instance lock and tray recovery route.
 * Keep it alive until after the QML windows are destroyed. Tray reachability
 * remains unverified on all platforms, so hiding controls is currently gated.
 * IPC is local-user-only, bounds clients and request length, and times out incomplete restore requests.
 */
class AppLifecycle final : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool recoveryAvailable READ recoveryAvailable NOTIFY recoveryAvailableChanged)
public:
  /** Result of attempting to start the per-user primary instance. */
  enum class Startup { primary, restored, unavailable };
  /** Construct on the GUI thread; key and lock directory are application-private. */
  explicit AppLifecycle(const QString& key, const QString& lockDirectory, QObject* parent = nullptr);
  // Destructor documented manually in the API reference due to Hawkmoth parsing.
  ~AppLifecycle() override;
  /** Become primary or ask the existing primary to restore its controls. A failed handoff never starts a competing instance. */
  Startup start();
  /** Create a GUI-thread warning explaining how to recover when relaunch cannot restore the existing instance. The caller owns and presents the dialog. */
  static std::unique_ptr<QMessageBox> handoffFailureDialog();
  /** Borrow the main QML window until its destruction; create a tray only when a recovery route is available. */
  void attach(QQuickWindow* controls);
  /** True only while the tray is present and the platform supports the guarded recovery path. */
  bool recoveryAvailable() const { return recoveryAvailable_; }
  /** Hide controls only while a recovery route is currently available. */
  Q_INVOKABLE bool hideControls();
  /** Restore, raise and activate the existing control window. */
  Q_INVOKABLE void showControls();
signals:
  /** Emitted before controls are restored if the tray recovery route disappears. */
  void recoveryAvailableChanged();
  /** Request that the UI unlock the overlay and show its controls. */
  void editOverlayRequested();
  /** Request that the UI hide the overlay and show its controls. */
  void hideOverlayRequested();
private:
  friend class AppLifecycleTest;
  static bool readAcknowledgement(QLocalSocket& socket, int timeoutMs);
  void refreshRecovery();
  void acceptConnections();
  void consumeRequest(QLocalSocket* socket);
  void acknowledgeRestore(QLocalSocket* socket);
  bool eventFilter(QObject* watched, QEvent* event) override;
  QString serverName_;
  QLockFile lock_;
  QLocalServer server_;
  QSet<QLocalSocket*> clients_;
  QPointer<QQuickWindow> controls_;
  std::unique_ptr<QMenu> menu_;
  std::unique_ptr<QSystemTrayIcon> tray_;
  QTimer recoveryTimer_;
  bool primary_{}, recoveryAvailable_{}, restorePending_{};
};
