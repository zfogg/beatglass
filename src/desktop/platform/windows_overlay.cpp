#include "windows_overlay.hpp"
#include <QEvent>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QScreen>
#include <QSettings>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <windows.h>

namespace beatglass::windows {
OverlayController::OverlayController(QQuickWindow* overlay, QWindow* control, QObject* parent, QSettings* settings)
    : QObject(parent), overlay_(overlay), control_(control), settings_(settings) {
  drag_timer_ = new QTimer(this);
  drag_timer_->setInterval(16);
  connect(drag_timer_, &QTimer::timeout, this, [this] {
    dragToCursor();
    if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) endDrag();
  });
  if (overlay_) {
    overlay_->setColor(Qt::transparent);
    connect(overlay_, &QObject::destroyed, this, [this] { overlay_ = nullptr; setClickThrough(false); });
    applyFlags();
    if (settings) {
      const auto savedPosition = settings->value(QStringLiteral("overlay/position"));
      if (savedPosition.canConvert<QPoint>()) overlay_->setPosition(savedPosition.toPoint());
      bool validOpacity = false;
      const auto opacity = settings->value(QStringLiteral("overlay/opacity")).toDouble(&validOpacity);
      if (validOpacity && std::isfinite(opacity)) setOpacity(opacity);
      keepOnScreen();
      connect(overlay_, &QWindow::xChanged, this, [this, settings] { if (overlay_) settings->setValue(QStringLiteral("overlay/position"), overlay_->position()); });
      connect(overlay_, &QWindow::yChanged, this, [this, settings] { if (overlay_) settings->setValue(QStringLiteral("overlay/position"), overlay_->position()); });
      connect(overlay_, &QWindow::opacityChanged, this, [settings](qreal value) { settings->setValue(QStringLiteral("overlay/opacity"), value); });
    }
  }
  if (control_) {
    control_->installEventFilter(this);
    connect(control_, &QObject::destroyed, this, [this] { control_ = nullptr; setClickThrough(false); });
  }
  connect(qGuiApp, &QGuiApplication::screenRemoved, this, [this](QScreen*) { QTimer::singleShot(0, this, [this] { keepOnScreen(); }); });
}
OverlayController::~OverlayController() { setClickThrough(false); if (control_) control_->removeEventFilter(this); }
void OverlayController::setOpacity(qreal opacity) {
  if (overlay_ && std::isfinite(opacity)) {
    const auto value = std::clamp(opacity, qreal(0), qreal(1));
    const bool changed = !qFuzzyCompare(overlay_->opacity(), value);
    overlay_->setOpacity(value);
    if (settings_) settings_->setValue(QStringLiteral("overlay/opacity"), value);
    if (changed) emit opacityChanged(value);
  }
}
qreal OverlayController::opacity() const { return overlay_ ? overlay_->opacity() : qreal(1); }
bool OverlayController::setClickThrough(bool enabled) {
  if (enabled && (!overlay_ || !control_ || overlay_.data() == control_.data() || !control_->isVisible() || control_->windowState() == Qt::WindowMinimized)) return false;
  const bool changed = click_through_ != enabled;
  if (enabled) endDrag();
  click_through_ = enabled;
  applyFlags();
  if (changed) emit clickThroughChanged(enabled);
  return true;
}
bool OverlayController::clickThrough() const { return click_through_; }
bool OverlayController::beginDrag() {
  POINT cursor{}; RECT bounds{};
  if (!overlay_ || click_through_ || !GetCursorPos(&cursor) || !GetWindowRect(reinterpret_cast<HWND>(overlay_->winId()), &bounds)) return false;
  cursor_x_ = cursor.x; cursor_y_ = cursor.y; window_x_ = bounds.left; window_y_ = bounds.top;
  dragging_ = true;
  drag_timer_->start();
  return true;
}
void OverlayController::dragToCursor() {
  POINT cursor{};
  if (!dragging_ || !overlay_ || !GetCursorPos(&cursor)) return;
  SetWindowPos(reinterpret_cast<HWND>(overlay_->winId()), nullptr,
               window_x_ + cursor.x - cursor_x_, window_y_ + cursor.y - cursor_y_,
               0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}
void OverlayController::endDrag() { dragging_ = false; drag_timer_->stop(); }
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
      (event->type() == QEvent::WindowStateChange && control_->windowState() == Qt::WindowMinimized))) { endDrag(); setClickThrough(false); }
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
