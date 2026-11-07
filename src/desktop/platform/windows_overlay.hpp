#pragma once
#include <QObject>
#include <QPointer>

class QEvent;
class QQuickWindow;
class QScreen;
class QWindow;

namespace beatglass::windows {
/** GUI-thread-only provisional window controller. The shared UI owns both windows and must provide visible unlock/hide controls. The controller never creates overlay content. */
class OverlayController final : public QObject {
  Q_OBJECT
public:
  /** Configure the overlay as transparent, frameless and always on top. The control window is the escape path and must outlive this controller or trigger automatic unlock. */
  OverlayController(QQuickWindow* overlay, QWindow* control, QObject* parent = nullptr);
  // Documented manually in the API reference due to Hawkmoth destructor parsing.
  ~OverlayController() override;
  /** Set whole-window opacity, clamped to the range zero through one. */
  void setOpacity(qreal opacity);
  /** Enable click-through only while the control window is visible and not minimized; return whether the requested state was applied. Hiding, minimizing, closing or destroying the control window unlocks the overlay. */
  bool setClickThrough(bool enabled);
  /** Return the current click-through state. */
  bool clickThrough() const;
  /** Move and resize an overlay into an available screen's logical-pixel geometry. No action occurs if no screen is available. */
  void keepOnScreen();
protected:
  /** Unlock if the control window is hidden, minimized or closed. */
  bool eventFilter(QObject* watched, QEvent* event) override;
private:
  void applyFlags();
  QPointer<QQuickWindow> overlay_;
  QPointer<QWindow> control_;
  bool click_through_{};
};
}
