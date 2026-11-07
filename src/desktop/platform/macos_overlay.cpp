#include "macos_overlay.hpp"
#include <QEvent>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QPlatformSurfaceEvent>
#include <QScreen>
#include <QTimer>
#include <algorithm>
#include <cmath>

namespace beatglass::macos {
OverlayController::OverlayController(std::unique_ptr<OverlayNative> native, QObject* parent)
    : QObject(parent), native_(std::move(native)) {
  connect(qGuiApp, &QGuiApplication::screenRemoved, this, [this](QScreen*) {
    QTimer::singleShot(0, this, [this] { keepOnScreen(); });
  });
}
OverlayController::~OverlayController() { detach(); }
void OverlayController::setAvailable(bool value) {
  if (available_ == value) return;
  available_ = value; emit availableChanged();
}
bool OverlayController::attach(QQuickWindow* overlay, QWindow* control) {
  detach();
  if (!native_ || !overlay || !control || overlay == control) return false;
  overlay_ = overlay; control_ = control;
  overlay_->setColor(Qt::transparent);
  auto flags = overlay_->flags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::WindowDoesNotAcceptFocus;
  flags.setFlag(Qt::WindowTransparentForInput, false);
  overlay_->setFlags(flags);
  overlay_->installEventFilter(this); control_->installEventFilter(this);
  connect(overlay_, &QObject::destroyed, this, [this] { overlay_ = nullptr; detach(); });
  connect(control_, &QObject::destroyed, this, [this] { control_ = nullptr; detach(); });
  overlay_->create();
  if (!native_->apply(overlay_, false)) { detach(); return false; }
  setAvailable(true); return true;
}
void OverlayController::detach() {
  if (overlay_) {
    if (native_ && !native_->apply(overlay_, false)) overlay_->hide();
    overlay_->hide();
    overlay_->removeEventFilter(this);
    disconnect(overlay_, nullptr, this, nullptr);
  }
  if (control_) { control_->removeEventFilter(this); disconnect(control_, nullptr, this, nullptr); }
  overlay_ = nullptr; control_ = nullptr;
  if (click_through_) { click_through_ = false; emit clickThroughChanged(); }
  setAvailable(false);
}
bool OverlayController::showOverlay() {
  if (!available_ || !overlay_ || !control_ || !control_->isVisible() || control_->windowState() == Qt::WindowMinimized) return false;
  keepOnScreen(); overlay_->show();
  if (!native_->apply(overlay_, click_through_)) {
    overlay_->hide(); setAvailable(false);
    if (click_through_) { click_through_ = false; emit clickThroughChanged(); }
    return false;
  }
  return true;
}
void OverlayController::hideOverlay() {
  setClickThrough(false);
  if (overlay_) overlay_->hide();
}
void OverlayController::setOpacity(qreal value) {
  if (overlay_ && std::isfinite(value)) overlay_->setOpacity(std::clamp(value, qreal(0), qreal(1)));
}
bool OverlayController::setClickThrough(bool enabled) {
  if (!overlay_ || !native_) return !enabled;
  if (enabled && (!available_ || !control_ || overlay_.data() == control_.data() || !control_->isVisible() ||
                  control_->windowState() == Qt::WindowMinimized || !overlay_->isVisible())) return false;
  if (!native_->apply(overlay_, enabled)) {
    // The actual input mode is unknown on failure: hide before declaring the feature unavailable.
    overlay_->hide(); setAvailable(false);
    if (click_through_) { click_through_ = false; emit clickThroughChanged(); }
    return false;
  }
  if (click_through_ != enabled) { click_through_ = enabled; emit clickThroughChanged(); }
  return true;
}
bool OverlayController::clickThrough() const { return click_through_; }
bool OverlayController::available() const { return available_; }
bool OverlayController::eventFilter(QObject* watched, QEvent* event) {
  if (watched == control_ && (event->type() == QEvent::Hide || event->type() == QEvent::Close ||
      (event->type() == QEvent::WindowStateChange && control_->windowState() == Qt::WindowMinimized))) setClickThrough(false);
  if (watched == overlay_ && event->type() == QEvent::PlatformSurface &&
      static_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated) {
    const QPointer<QQuickWindow> expected = overlay_;
    QTimer::singleShot(0, this, [this, expected] {
      if (expected && expected == overlay_ && available_ && !native_->apply(expected, click_through_)) {
        expected->hide(); setAvailable(false);
        if (click_through_) { click_through_ = false; emit clickThroughChanged(); }
      }
    });
  }
  return QObject::eventFilter(watched, event);
}
void OverlayController::keepOnScreen() {
  if (!overlay_) return;
  auto* screen = QGuiApplication::screenAt(overlay_->geometry().center());
  if (!screen) screen = QGuiApplication::primaryScreen();
  if (!screen) return;
  const auto bounds = screen->availableGeometry();
  if (bounds.isEmpty()) return;
  auto geometry = overlay_->geometry();
  geometry.setSize(geometry.size().boundedTo(bounds.size()));
  geometry.moveLeft(std::clamp(geometry.left(), bounds.left(), bounds.right() - geometry.width() + 1));
  geometry.moveTop(std::clamp(geometry.top(), bounds.top(), bounds.bottom() - geometry.height() + 1));
  overlay_->setGeometry(geometry);
}
}
