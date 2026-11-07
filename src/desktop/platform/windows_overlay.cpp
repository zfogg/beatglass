#include "windows_overlay.hpp"
#include <QEvent>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>
#include <algorithm>
#include <cmath>

namespace beatglass::windows {
OverlayController::OverlayController(QQuickWindow* overlay, QWindow* control, QObject* parent)
    : QObject(parent), overlay_(overlay), control_(control) {
  if (overlay_) {
    overlay_->setColor(Qt::transparent);
    connect(overlay_, &QObject::destroyed, this, [this] { overlay_ = nullptr; setClickThrough(false); });
    applyFlags();
  }
  if (control_) {
    control_->installEventFilter(this);
    connect(control_, &QObject::destroyed, this, [this] { control_ = nullptr; setClickThrough(false); });
  }
  connect(qGuiApp, &QGuiApplication::screenRemoved, this, [this](QScreen*) { QTimer::singleShot(0, this, [this] { keepOnScreen(); }); });
}
OverlayController::~OverlayController() { setClickThrough(false); if (control_) control_->removeEventFilter(this); }
void OverlayController::setOpacity(qreal opacity) {
  if (overlay_ && std::isfinite(opacity)) overlay_->setOpacity(std::clamp(opacity, qreal(0), qreal(1)));
}
bool OverlayController::setClickThrough(bool enabled) {
  if (enabled && (!overlay_ || !control_ || !control_->isVisible() || control_->windowState() == Qt::WindowMinimized)) return false;
  const bool changed = click_through_ != enabled;
  click_through_ = enabled;
  applyFlags();
  if (changed) emit clickThroughChanged(enabled);
  return true;
}
bool OverlayController::clickThrough() const { return click_through_; }
void OverlayController::applyFlags() {
  if (!overlay_) return;
  auto flags = overlay_->flags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool;
  flags.setFlag(Qt::WindowTransparentForInput, click_through_);
  const bool visible = overlay_->isVisible();
  overlay_->setFlags(flags);
  if (visible && !overlay_->isVisible()) overlay_->show();
}
bool OverlayController::eventFilter(QObject* watched, QEvent* event) {
  if (watched == control_ && (event->type() == QEvent::Hide || event->type() == QEvent::Close ||
      (event->type() == QEvent::WindowStateChange && control_->windowState() == Qt::WindowMinimized))) setClickThrough(false);
  return QObject::eventFilter(watched, event);
}
void OverlayController::keepOnScreen() {
  if (!overlay_) return;
  auto* screen = QGuiApplication::screenAt(overlay_->geometry().center());
  if (!screen) screen = QGuiApplication::primaryScreen();
  if (!screen) return;
  const auto bounds = screen->availableGeometry();
  auto geometry = overlay_->geometry();
  geometry.setSize(geometry.size().boundedTo(bounds.size()));
  geometry.moveLeft(std::clamp(geometry.left(), bounds.left(), bounds.right() - geometry.width() + 1));
  geometry.moveTop(std::clamp(geometry.top(), bounds.top(), bounds.bottom() - geometry.height() + 1));
  overlay_->setGeometry(geometry);
}
}
