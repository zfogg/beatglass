#pragma once
#include <QObject>
#include <QPointer>

class QEvent;
class QQuickWindow;
class QScreen;
class QSettings;
class QTimer;
class QWindow;

namespace beatglass::windows {
/** GUI-thread-only provisional window controller. The shared UI owns both windows and must provide visible unlock/hide controls. The controller never creates overlay content. */
class OverlayController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool clickThrough READ clickThrough NOTIFY clickThroughChanged)
  Q_PROPERTY(qreal opacity READ opacity NOTIFY opacityChanged)
public:
  /** Configure the overlay as transparent, frameless and always on top. The control window is the escape path and must outlive this controller or trigger automatic unlock. Optional settings are caller-owned, must outlive the controller and are read and written only on the GUI thread. Saved position and opacity are restored; visibility remains caller-controlled and click-through starts disabled. */
  OverlayController(QQuickWindow* overlay, QWindow* control, QObject* parent = nullptr, QSettings* settings = nullptr);
  // Documented manually in the API reference due to Hawkmoth destructor parsing.
  ~OverlayController() override;
  /** Set whole-window opacity, clamped to the range zero through one. */
  Q_INVOKABLE void setOpacity(qreal opacity);
  /** Return the current whole-window opacity, or one if the overlay no longer exists. */
  qreal opacity() const;
  /** Enable click-through only with a distinct visible control window or an established external recovery route. Loss of both unlocks the overlay. */
  Q_INVOKABLE bool setClickThrough(bool enabled);
  /** Set the GUI-owned tray recovery guarantee. Revocation immediately unlocks if controls are hidden or minimized. */
  void setExternalRecoveryAvailable(bool available);
  /** Return the current click-through state. */
  bool clickThrough() const;
  /** Move and resize an overlay into an available screen's logical-pixel geometry. No action occurs if no screen is available. */
  Q_INVOKABLE void keepOnScreen();
  /** Begin a GUI-thread drag using the current native cursor and window positions. A GUI timer follows the cursor until button release, hide or explicit end. Returns false when either position is unavailable. */
  Q_INVOKABLE bool beginDrag();
  /** Move an active drag to the current native cursor position, including across mixed-DPI monitors. */
  Q_INVOKABLE void dragToCursor();
  /** End a GUI-thread drag. */
  Q_INVOKABLE void endDrag();
  /** Emitted on the GUI thread when click-through changes, including automatic unlock. */
  Q_SIGNAL void clickThroughChanged(bool enabled);
  /** Emitted on the GUI thread when the controller changes opacity. */
  Q_SIGNAL void opacityChanged(qreal opacity);
protected:
  /** Unlock on control-window loss unless external recovery is established. */
  bool eventFilter(QObject* watched, QEvent* event) override;
private:
  void applyFlags();
  QPointer<QQuickWindow> overlay_;
  QPointer<QWindow> control_;
  QSettings* settings_{};
  QTimer* drag_timer_{};
  bool dragging_{};
  long cursor_x_{}, cursor_y_{}, window_x_{}, window_y_{};
  bool click_through_{};
  bool external_recovery_{};
};
}
