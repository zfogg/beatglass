#pragma once
#include <QObject>
#include <memory>
class QQuickWindow;
class QSettings;
class QWindow;
namespace beatglass::windows { class OverlayController; }
/** GUI-thread wrapper borrowing both windows, which must outlive it. It exposes automatic click-through unlock; monitor preferences and global shortcuts remain unimplemented. */
class WindowsOverlay : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool clickThrough READ clickThrough NOTIFY clickThroughChanged)
  Q_PROPERTY(qreal opacity READ opacity NOTIFY opacityChanged)
public:
  /** Configure distinct windows on the GUI thread. Optional caller-owned settings must outlive this bridge; position and opacity are restored and saved on the GUI thread. */
  WindowsOverlay(QQuickWindow* overlay, QWindow* control, QObject* parent = nullptr, QSettings* settings = nullptr);
  // Documented manually in the API index for Hawkmoth destructor parsing.
  ~WindowsOverlay() override;
  /** Current platform click-through state. */
  bool clickThrough() const;
  /** Request click-through; false means the controller rejected the request. */
  Q_INVOKABLE bool setClickThrough(bool enabled);
  /** Clamp and apply the window opacity. */
  Q_INVOKABLE void setOpacity(qreal opacity);
  /** Current restored or selected window opacity. */
  qreal opacity() const;
  /** Move the overlay inside available logical screen geometry. */
  Q_INVOKABLE void keepOnScreen();
  /** Begin a GUI-thread native cursor drag; false indicates unavailable state. */
  Q_INVOKABLE bool beginDrag();
  /** Move the active drag to the current native cursor position. */
  Q_INVOKABLE void dragToCursor();
  /** Stop any active drag. */
  Q_INVOKABLE void endDrag();
signals:
  /** Emitted on the GUI thread for explicit or automatic state changes. */
  void clickThroughChanged();
  /** Emitted on the GUI thread when opacity changes. */
  void opacityChanged();
private:
  std::unique_ptr<beatglass::windows::OverlayController> controller_;
};
