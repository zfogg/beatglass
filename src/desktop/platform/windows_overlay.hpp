#pragma once
#include <QObject>
#include <QPointer>
#include <QRect>
#include <QString>
#include <QStringList>

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
  Q_PROPERTY(QRect geometry READ geometry NOTIFY geometryChanged)
  Q_PROPERTY(QString displayName READ displayName NOTIFY geometryChanged)
  Q_PROPERTY(QStringList screenNames READ screenNames NOTIFY screensChanged)
  Q_PROPERTY(int screenIndex READ screenIndex NOTIFY screensChanged)
  Q_PROPERTY(bool available READ available CONSTANT)
public:
  /** Configure the overlay as transparent, frameless and always on top. The control window is the escape path and must outlive this controller or trigger automatic unlock. Optional settings are caller-owned, must outlive the controller and are read and written only on the GUI thread. Saved position and opacity are restored; visibility remains caller-controlled and click-through starts disabled. */
  OverlayController(QQuickWindow* overlay, QWindow* control, QObject* parent = nullptr, QSettings* settings = nullptr);
  // Documented manually in the API reference due to Hawkmoth destructor parsing.
  ~OverlayController() override;
  /** Set whole-window opacity, clamped to the visible range 0.2 through one. Return false when the overlay is unavailable or input is non-finite. */
  Q_INVOKABLE bool setOpacity(qreal opacity);
  /** Return the current whole-window opacity, or one if the overlay no longer exists. */
  qreal opacity() const;
  /** Return whether both overlay and escape-control windows are available. */
  bool available() const;
  /** Return logical overlay geometry, or an empty rectangle when unavailable. */
  QRect geometry() const;
  /** Return the current display name, or empty when unavailable. */
  QString displayName() const;
  /** Return current display labels in Qt screen order. Names are presentation labels, not persistent identifiers. */
  QStringList screenNames() const;
  /** Return the overlay display index, or -1 when unavailable. */
  int screenIndex() const;
  /** Move and center the overlay on the selected display, clamping to its work area. */
  Q_INVOKABLE bool moveToScreen(int index);
  /** Show the overlay after clamping it to a current display. */
  Q_INVOKABLE bool showOverlay();
  /** Hide the overlay and clear click-through state. */
  Q_INVOKABLE void hideOverlay();
  /** Resize the overlay to positive logical dimensions, clamped on screen. */
  Q_INVOKABLE bool resizeOverlay(int width, int height);
  /** Enable click-through only while the distinct control window is visible and not minimized; return whether the requested state was applied. Hiding, minimizing, closing or destroying the control window unlocks the overlay. */
  Q_INVOKABLE bool setClickThrough(bool enabled);
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
  Q_SIGNAL void geometryChanged();
  Q_SIGNAL void screensChanged();
protected:
  /** Unlock if the control window is hidden, minimized or closed. */
  bool eventFilter(QObject* watched, QEvent* event) override;
private:
  void applyFlags();
  /** Persist the current native display target after monitor/layout recovery. */
  void persistCurrentDisplay();
  QPointer<QQuickWindow> overlay_;
  QPointer<QWindow> control_;
  QSettings* settings_{};
  QTimer* drag_timer_{};
  bool dragging_{};
  long cursor_x_{}, cursor_y_{}, window_x_{}, window_y_{};
  bool click_through_{};
};
}
