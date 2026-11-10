#pragma once
#include <QObject>
#include <QPointer>
#include <QRect>
#include <QStringList>
#include <QSettings>
#include <QTimer>
#include <memory>

class QEvent;
class QQuickWindow;
class QScreen;
class QWindow;

namespace beatglass::macos {
/** Private GUI-thread native-window operations. The implementation never owns the Qt window. A false result means native behavior was not applied. */
class OverlayNative {
public:
  // Destructor documented manually in the API reference due to Hawkmoth parsing.
  virtual ~OverlayNative() = default;
  /** Configure the native window, including its requested input mode. */
  virtual bool apply(QQuickWindow* window, bool clickThrough) = 0;
  /** Clear adapter-side input-mode state after the Qt window has been destroyed. */
  virtual void reset() noexcept {}
};
/** Create the AppKit implementation on macOS. Caller owns the result. */
std::unique_ptr<OverlayNative> makeOverlayNative();

/** Provisional GUI-thread-only overlay controller. The caller owns both distinct windows and supplies a separate visible control-window escape path. An optional owned QSettings store records geometry and opacity on the GUI thread; write failures are not currently surfaced, and click-through is never restored. AppKit hints do not guarantee visibility over other applications or fullscreen Spaces. */
class OverlayController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool clickThrough READ clickThrough NOTIFY clickThroughChanged)
  Q_PROPERTY(bool available READ available NOTIFY availableChanged)
  Q_PROPERTY(QStringList screenNames READ screenNames NOTIFY screensChanged)
  Q_PROPERTY(int screenIndex READ screenIndex NOTIFY screensChanged)
  Q_PROPERTY(qreal opacity READ opacity NOTIFY opacityChanged)
  Q_PROPERTY(QRect geometry READ geometry NOTIFY geometryChanged)
  Q_PROPERTY(QString displayName READ displayName NOTIFY geometryChanged)
public:
  /** Own the native adapter and optional settings store; the Qt parent, if any, owns this controller. All methods run on the GUI thread. */
  explicit OverlayController(std::unique_ptr<OverlayNative> native, QObject* parent = nullptr, std::unique_ptr<QSettings> settings = {});
  // Destructor documented manually in the API reference due to Hawkmoth parsing.
  ~OverlayController() override;
  /** Attach distinct caller-owned windows. Returns false on invalid arguments or failed native setup. */
  Q_INVOKABLE bool attach(QQuickWindow* overlay, QWindow* control);
  /** Show without requesting focus; false indicates setup failure. */
  Q_INVOKABLE bool showOverlay();
  /** Hide and attempt to unlock the overlay. */
  /** Hide and attempt to unlock the overlay. Return false when unattached. */
  Q_INVOKABLE bool hideOverlay();
  /** Set whole-window opacity, clamped to the visible range [0.2,1]. Return false when unavailable or input is non-finite. */
  Q_INVOKABLE bool setOpacity(qreal value);
  /** Return the current overlay opacity or the default before attachment. */
  qreal opacity() const;
  /** Return logical overlay geometry, or an empty rectangle when unavailable. */
  QRect geometry() const;
  /** Return the current display name, or empty when unavailable. */
  QString displayName() const;
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
  /** Index of the overlay's current display in screenNames(), or -1 when unattached or no display contains its center. Indices are session-local. */
  int screenIndex() const;
  /** Move the overlay to the selected current display and clamp it to its available geometry. Return false if the display or overlay is unavailable. */
  Q_INVOKABLE bool moveToScreen(int index);
  /** Request a positive logical-pixel size, then clamp to the current display. Qt minimum-size constraints still apply. */
  Q_INVOKABLE bool resizeOverlay(int width, int height);
  /** Emitted when requested input mode changes, including automatic unlock. */
  Q_SIGNAL void clickThroughChanged();
  /** Emitted when native availability changes. */
  Q_SIGNAL void availableChanged();
  /** Emitted when displays, their geometry, or overlay position change; callers must refresh any selected index. */
  Q_SIGNAL void screensChanged();
  /** Emitted when the overlay opacity changes. */
  Q_SIGNAL void opacityChanged();
  Q_SIGNAL void geometryChanged();
protected:
  /** Reconfigure on surface recreation and unlock on control hide, close or minimization. */
  bool eventFilter(QObject* watched, QEvent* event) override;
private:
  void setAvailable(bool available);
  void detach();
  void scheduleSave();
  void saveSettings();
  void observeScreen(QScreen* screen);
  void scheduleScreenUpdate();
  std::unique_ptr<OverlayNative> native_;
  std::unique_ptr<QSettings> settings_;
  QTimer save_timer_;
  QTimer screen_timer_;
  QPointer<QQuickWindow> overlay_;
  QPointer<QWindow> control_;
  QRect saved_geometry_;
  qreal saved_opacity_{0.9};
  bool available_{}, click_through_{}, restoring_{}, have_snapshot_{}, updating_screens_{};
};
}
