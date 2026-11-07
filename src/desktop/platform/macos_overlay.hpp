#pragma once
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <memory>

class QEvent;
class QQuickWindow;
class QWindow;

namespace beatglass::macos {
/** Private GUI-thread native-window operations. The implementation never owns the Qt window. A false result means native behavior was not applied. */
class OverlayNative {
public:
  // Destructor documented manually in the API reference due to Hawkmoth parsing.
  virtual ~OverlayNative() = default;
  /** Configure the native window, including its requested input mode. */
  virtual bool apply(QQuickWindow* window, bool clickThrough) = 0;
};
/** Create the AppKit implementation on macOS. Caller owns the result. */
std::unique_ptr<OverlayNative> makeOverlayNative();

/** Provisional GUI-thread-only overlay controller. The caller owns both distinct windows and supplies a separate visible control-window escape path. AppKit hints do not guarantee visibility over other applications or fullscreen Spaces. */
class OverlayController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool clickThrough READ clickThrough NOTIFY clickThroughChanged)
  Q_PROPERTY(bool available READ available NOTIFY availableChanged)
  Q_PROPERTY(QStringList screenNames READ screenNames NOTIFY screensChanged)
public:
  /** Own the native adapter; the Qt parent, if any, owns this controller. All methods run on the GUI thread. */
  explicit OverlayController(std::unique_ptr<OverlayNative> native, QObject* parent = nullptr);
  // Destructor documented manually in the API reference due to Hawkmoth parsing.
  ~OverlayController() override;
  /** Attach distinct caller-owned windows. Returns false on invalid arguments or failed native setup. */
  Q_INVOKABLE bool attach(QQuickWindow* overlay, QWindow* control);
  /** Show without requesting focus; false indicates setup failure. */
  Q_INVOKABLE bool showOverlay();
  /** Hide and attempt to unlock the overlay. */
  Q_INVOKABLE void hideOverlay();
  /** Set whole-window opacity, clamped to [0,1]; ignore non-finite input. */
  Q_INVOKABLE void setOpacity(qreal value);
  /** Apply input transparency only when a separate control window is visible and not minimized. A failed unlock hides the overlay. */
  Q_INVOKABLE bool setClickThrough(bool enabled);
  /** True if click-through was successfully requested and remains active. */
  bool clickThrough() const;
  /** True while attached to valid windows and native setup succeeded. */
  bool available() const;
  /** Clamp the overlay to an available display in Qt logical coordinates, if a display exists. */
  Q_INVOKABLE void keepOnScreen();
  /** Return current display labels in Qt's screen order; names are presentation labels, not persistent identifiers. */
  QStringList screenNames() const;
  /** Move the overlay to the selected current display and clamp it to its available geometry. Return false if the display or overlay is unavailable. */
  Q_INVOKABLE bool moveToScreen(int index);
  /** Request a positive logical-pixel size, then clamp to the current display. Qt minimum-size constraints still apply. */
  Q_INVOKABLE bool resizeOverlay(int width, int height);
  /** Emitted when requested input mode changes, including automatic unlock. */
  Q_SIGNAL void clickThroughChanged();
  /** Emitted when native availability changes. */
  Q_SIGNAL void availableChanged();
  /** Emitted on display addition or removal; callers must refresh any selected index. */
  Q_SIGNAL void screensChanged();
protected:
  /** Reconfigure on surface recreation and unlock on control hide, close or minimization. */
  bool eventFilter(QObject* watched, QEvent* event) override;
private:
  void setAvailable(bool available);
  void detach();
  std::unique_ptr<OverlayNative> native_;
  QPointer<QQuickWindow> overlay_;
  QPointer<QWindow> control_;
  bool available_{}, click_through_{};
};
}
