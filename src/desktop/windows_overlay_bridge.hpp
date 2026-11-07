#pragma once
#include <QObject>
#include <memory>
class QQuickWindow;
class QWindow;
namespace beatglass::windows { class OverlayController; }
/** GUI-thread wrapper borrowing both windows, which must outlive it. It exposes automatic click-through unlock; monitor preferences and global shortcuts remain unimplemented. */
class WindowsOverlay : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool clickThrough READ clickThrough NOTIFY clickThroughChanged)
public:
  /** Configure a distinct overlay and control window on their owning GUI thread. */
  WindowsOverlay(QQuickWindow* overlay, QWindow* control, QObject* parent = nullptr);
  // Documented manually in the API index for Hawkmoth destructor parsing.
  ~WindowsOverlay() override;
  /** Current platform click-through state. */
  bool clickThrough() const;
  /** Request click-through; false means the controller rejected the request. */
  Q_INVOKABLE bool setClickThrough(bool enabled);
  /** Clamp and apply the window opacity. */
  Q_INVOKABLE void setOpacity(qreal opacity);
  /** Move the overlay inside available logical screen geometry. */
  Q_INVOKABLE void keepOnScreen();
signals:
  /** Emitted on the GUI thread for explicit or automatic state changes. */
  void clickThroughChanged();
private:
  std::unique_ptr<beatglass::windows::OverlayController> controller_;
};
