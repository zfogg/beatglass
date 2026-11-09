#include "windows_overlay.hpp"
#include "windows_overlay_policy.hpp"
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
    connect(overlay_, &QWindow::xChanged, this, &OverlayController::geometryChanged);
    connect(overlay_, &QWindow::yChanged, this, &OverlayController::geometryChanged);
    connect(overlay_, &QWindow::widthChanged, this, &OverlayController::geometryChanged);
    connect(overlay_, &QWindow::heightChanged, this, &OverlayController::geometryChanged);
    overlay_->setColor(Qt::transparent);
    overlay_->installEventFilter(this);
    connect(overlay_, &QObject::destroyed, this, [this] { overlay_ = nullptr; setClickThrough(false); });
    applyFlags();
    if (settings) {
      const auto savedPosition = settings->value(QStringLiteral("overlay/position"));
      if (savedPosition.canConvert<QPoint>()) overlay_->setPosition(savedPosition.toPoint());
      const auto savedSize = settings->value(QStringLiteral("overlay/size"));
      if (savedSize.canConvert<QSize>()) {
        const auto size = savedSize.toSize();
        if (size.width() > 0 && size.height() > 0) overlay_->resize(size);
      }
      const auto savedDisplay = settings->value(QStringLiteral("overlay/displayName")).toString();
      if (!savedDisplay.isEmpty()) {
        for (auto* screen : QGuiApplication::screens()) {
          if (screen->name() == savedDisplay) { overlay_->setScreen(screen); break; }
        }
      }
      bool validOpacity = false;
      const auto opacity = settings->value(QStringLiteral("overlay/opacity")).toDouble(&validOpacity);
      if (validOpacity && std::isfinite(opacity)) setOpacity(opacity);
      keepOnScreen();
      connect(overlay_, &QWindow::xChanged, this, [this, settings] { if (overlay_) settings->setValue(QStringLiteral("overlay/position"), overlay_->position()); });
      connect(overlay_, &QWindow::yChanged, this, [this, settings] { if (overlay_) settings->setValue(QStringLiteral("overlay/position"), overlay_->position()); });
      connect(overlay_, &QWindow::widthChanged, this, [this, settings] { if (overlay_) settings->setValue(QStringLiteral("overlay/size"), overlay_->size()); });
      connect(overlay_, &QWindow::heightChanged, this, [this, settings] { if (overlay_) settings->setValue(QStringLiteral("overlay/size"), overlay_->size()); });
      connect(overlay_, &QWindow::screenChanged, this, [this, settings](QScreen* screen) {
        if (screen) settings->setValue(QStringLiteral("overlay/displayName"), screen->name());
      });
      connect(overlay_, &QWindow::opacityChanged, this, [settings](qreal value) { settings->setValue(QStringLiteral("overlay/opacity"), value); });
    }
  }
  if (control_) {
    control_->installEventFilter(this);
    connect(control_, &QObject::destroyed, this, [this] { control_ = nullptr; setClickThrough(false); });
  }
  connect(qGuiApp, &QGuiApplication::screenRemoved, this, [this](QScreen*) { QTimer::singleShot(0, this, [this] { keepOnScreen(); persistCurrentDisplay(); emit geometryChanged(); emit screensChanged(); }); });
  connect(qGuiApp, &QGuiApplication::screenAdded, this, [this](QScreen*) { QTimer::singleShot(0, this, [this] { keepOnScreen(); persistCurrentDisplay(); emit geometryChanged(); emit screensChanged(); }); });
  if (overlay_) {
    connect(overlay_, &QWindow::screenChanged, this, [this](QScreen*) {
      QTimer::singleShot(0, this, [this] { keepOnScreen(); persistCurrentDisplay(); emit geometryChanged(); emit screensChanged(); });
    });
  }
}
OverlayController::~OverlayController() { setClickThrough(false); if (overlay_) overlay_->removeEventFilter(this); if (control_) control_->removeEventFilter(this); }
bool OverlayController::setOpacity(qreal opacity) {
  if (overlay_ && std::isfinite(opacity)) {
    const auto value = std::clamp(opacity, qreal(0.2), qreal(1));
    const bool changed = !qFuzzyCompare(overlay_->opacity(), value);
    overlay_->setOpacity(value);
    if (settings_) settings_->setValue(QStringLiteral("overlay/opacity"), value);
    if (changed) emit opacityChanged(value);
    return true;
  }
  return false;
}
qreal OverlayController::opacity() const { return overlay_ ? overlay_->opacity() : qreal(1); }
void OverlayController::persistCurrentDisplay() {
  if (!settings_ || !overlay_) return;
  if (auto* screen = QGuiApplication::screenAt(overlay_->geometry().center()))
    settings_->setValue(QStringLiteral("overlay/displayName"), screen->name());
}
bool OverlayController::available() const { return overlay_ && control_; }
QRect OverlayController::geometry() const { return overlay_ ? overlay_->geometry() : QRect{}; }
QString OverlayController::displayName() const { auto* screen = overlay_ ? QGuiApplication::screenAt(overlay_->geometry().center()) : nullptr; return screen ? screen->name() : QString{}; }
QStringList OverlayController::screenNames() const {
  QStringList names;
  for (const auto* screen : QGuiApplication::screens())
    names.append(screen->name().isEmpty() ? QStringLiteral("Display %1").arg(names.size() + 1) : screen->name());
  return names;
}
int OverlayController::screenIndex() const {
  if (!overlay_) return -1;
  return QGuiApplication::screens().indexOf(QGuiApplication::screenAt(overlay_->geometry().center()));
}
bool OverlayController::moveToScreen(int index) {
  const auto screens = QGuiApplication::screens();
  if (!overlay_ || index < 0 || index >= screens.size()) return false;
  auto* screen = screens.at(index);
  const auto bounds = screen->availableGeometry();
  if (bounds.isEmpty()) return false;
  const auto size = overlay_->size().boundedTo(bounds.size());
  overlay_->setScreen(screen);
  if (settings_) settings_->setValue(QStringLiteral("overlay/displayName"), screen->name());
  overlay_->setGeometry(QRect(bounds.center() - QPoint(size.width() / 2, size.height() / 2), size));
  keepOnScreen();
  emit screensChanged();
  return true;
}
bool OverlayController::showOverlay() {
  if (!available()) return false;
  keepOnScreen();
  overlay_->show();
  return true;
}
void OverlayController::hideOverlay() {
  setClickThrough(false);
  if (overlay_) overlay_->hide();
}
bool OverlayController::resizeOverlay(int width, int height) {
  if (!overlay_ || width <= 0 || height <= 0) return false;
  overlay_->resize(width, height);
  if (settings_) settings_->setValue(QStringLiteral("overlay/size"), overlay_->size());
  keepOnScreen();
  return true;
}
bool OverlayController::setClickThrough(bool enabled) {
  if (enabled) {
    if (!overlay_ || !overlay_->isVisible()) return false;
    const auto control_state = !control_ ? OverlayControlState::unavailable :
        control_->isVisible() ? (control_->windowState() == Qt::WindowMinimized ? OverlayControlState::minimized : OverlayControlState::visible) : OverlayControlState::hidden;
    if (!canEnableClickThrough(static_cast<bool>(overlay_), overlay_ && control_ && overlay_.data() == control_.data(), control_state)) return false;
  }
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
  if (!canBeginDrag(static_cast<bool>(overlay_), overlay_ && overlay_->isVisible(), click_through_) ||
      !GetCursorPos(&cursor) || !GetWindowRect(reinterpret_cast<HWND>(overlay_->winId()), &bounds)) return false;
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
void OverlayController::endDrag() {
  const bool wasDragging = dragging_;
  dragging_ = false; drag_timer_->stop();
  if (wasDragging && settings_ && overlay_) settings_->setValue(QStringLiteral("overlay/position"), overlay_->position());
}
void OverlayController::applyFlags() {
  if (!overlay_) return;
  auto flags = overlay_->flags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool;
  flags.setFlag(Qt::WindowTransparentForInput, click_through_);
  const bool visible = overlay_->isVisible();
  overlay_->setFlags(flags);
  if (visible && !overlay_->isVisible()) overlay_->show();
}
bool OverlayController::eventFilter(QObject* watched, QEvent* event) {
  if (watched == overlay_ && (event->type() == QEvent::Hide || event->type() == QEvent::Close)) {
    endDrag();
    setClickThrough(false);
  }
  if (watched == control_ && (event->type() == QEvent::Hide || event->type() == QEvent::Close ||
      (event->type() == QEvent::WindowStateChange && control_->windowState() == Qt::WindowMinimized))) { endDrag(); setClickThrough(false); }
  if (watched == control_ && event->type() == QEvent::Close && overlay_) overlay_->hide();
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
  if (settings_) settings_->setValue(QStringLiteral("overlay/position"), geometry.topLeft());
}
}
