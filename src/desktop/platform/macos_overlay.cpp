#include "macos_overlay.hpp"
#include <QEvent>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QPlatformSurfaceEvent>
#include <QScreen>
#include <QSettings>
#include <QTimer>
#include <algorithm>
#include <cmath>

namespace beatglass::macos {
OverlayController::OverlayController(std::unique_ptr<OverlayNative> native, QObject* parent, std::unique_ptr<QSettings> settings)
    : QObject(parent), native_(std::move(native)), settings_(std::move(settings)) {
  save_timer_.setSingleShot(true); save_timer_.setInterval(250);
  connect(&save_timer_, &QTimer::timeout, this, [this] { saveSettings(); });
  screen_timer_.setSingleShot(true);
  connect(&screen_timer_, &QTimer::timeout, this, [this] {
    updating_screens_ = true; keepOnScreen(); updating_screens_ = false;
    scheduleSave();
    emit geometryChanged();
    emit screensChanged();
  });
  for (auto* screen : QGuiApplication::screens()) observeScreen(screen);
  connect(qGuiApp, &QGuiApplication::screenRemoved, this, [this](QScreen* screen) {
    disconnect(screen, nullptr, this, nullptr); scheduleScreenUpdate();
  });
  connect(qGuiApp, &QGuiApplication::screenAdded, this, [this](QScreen* screen) {
    observeScreen(screen); scheduleScreenUpdate();
  });
}
void OverlayController::observeScreen(QScreen* screen) {
  connect(screen, &QScreen::geometryChanged, this, [this](const QRect&) { scheduleScreenUpdate(); });
  connect(screen, &QScreen::availableGeometryChanged, this, [this](const QRect&) { scheduleScreenUpdate(); });
}
void OverlayController::scheduleScreenUpdate() { if (!screen_timer_.isActive()) screen_timer_.start(0); }
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
  connect(overlay_, &QObject::destroyed, this, [this] {
    if (native_) native_->reset();
    overlay_ = nullptr;
    detach();
  });
  connect(control_, &QObject::destroyed, this, [this] { control_ = nullptr; detach(); });
  connect(overlay_, &QWindow::xChanged, this, [this] { scheduleSave(); if (!updating_screens_) emit screensChanged(); });
  connect(overlay_, &QWindow::yChanged, this, [this] { scheduleSave(); if (!updating_screens_) emit screensChanged(); });
  connect(overlay_, &QWindow::widthChanged, this, [this] { scheduleSave(); if (!updating_screens_) emit screensChanged(); });
  connect(overlay_, &QWindow::heightChanged, this, [this] { scheduleSave(); if (!updating_screens_) emit screensChanged(); });
  connect(overlay_, &QWindow::opacityChanged, this, [this] { emit opacityChanged(); scheduleSave(); });
  overlay_->create();
  if (!native_->apply(overlay_, false)) { detach(); return false; }
  setAvailable(true);
  restoring_ = true;
  overlay_->setOpacity(0.9);
  bool restoredGeometry = false;
  if (settings_) {
    const auto savedDisplay = settings_->value(QStringLiteral("overlay/displayName")).toString();
    if (!savedDisplay.isEmpty()) {
      for (auto* screen : QGuiApplication::screens()) {
        if (screen->name() == savedDisplay) { overlay_->setScreen(screen); break; }
      }
    }
    const QRect saved = settings_->value(QStringLiteral("overlay/geometry")).toRect();
    if (saved.isValid() && saved.width() <= 10000 && saved.height() <= 10000) { overlay_->setGeometry(saved); restoredGeometry = true; }
    bool validOpacity = false;
    const auto savedOpacity = settings_->value(QStringLiteral("overlay/opacity"), 0.9).toDouble(&validOpacity);
    if (validOpacity && std::isfinite(savedOpacity)) overlay_->setOpacity(std::clamp(savedOpacity, 0.2, 1.0));
  }
  if (!restoredGeometry) {
    auto* screen = QGuiApplication::screenAt(control_->geometry().center());
    if (!screen) screen = control_->screen();
    const auto index = QGuiApplication::screens().indexOf(screen);
    if (index >= 0) moveToScreen(index);
  }
  keepOnScreen(); restoring_ = false; scheduleSave();
  emit opacityChanged(); emit screensChanged();
  return true;
}
void OverlayController::detach() {
  save_timer_.stop(); saveSettings();
  if (overlay_) {
    connect(overlay_, &QWindow::xChanged, this, &OverlayController::geometryChanged);
    connect(overlay_, &QWindow::yChanged, this, &OverlayController::geometryChanged);
    connect(overlay_, &QWindow::widthChanged, this, &OverlayController::geometryChanged);
    connect(overlay_, &QWindow::heightChanged, this, &OverlayController::geometryChanged);
    if (native_ && !native_->apply(overlay_, false)) overlay_->hide();
    overlay_->hide();
    overlay_->removeEventFilter(this);
    disconnect(overlay_, nullptr, this, nullptr);
  }
  if (control_) { control_->removeEventFilter(this); disconnect(control_, nullptr, this, nullptr); }
  overlay_ = nullptr; control_ = nullptr;
  emit screensChanged();
  have_snapshot_ = false;
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
bool OverlayController::hideOverlay() {
  setClickThrough(false);
  if (!overlay_) return false;
  overlay_->hide();
  return true;
}
bool OverlayController::setOpacity(qreal value) {
  if (overlay_ && std::isfinite(value)) {
    overlay_->setOpacity(std::clamp(value, qreal(0.2), qreal(1)));
    return true;
  }
  return false;
}
qreal OverlayController::opacity() const { return overlay_ ? overlay_->opacity() : qreal(0.9); }
QRect OverlayController::geometry() const { return overlay_ ? overlay_->geometry() : QRect{}; }
QString OverlayController::displayName() const { auto* screen = overlay_ ? QGuiApplication::screenAt(overlay_->geometry().center()) : nullptr; return screen ? screen->name() : QString{}; }
void OverlayController::scheduleSave() {
  if (!settings_ || !available_ || !overlay_ || restoring_) return;
  saved_geometry_ = overlay_->geometry(); saved_opacity_ = overlay_->opacity(); have_snapshot_ = true;
  save_timer_.start();
}
void OverlayController::saveSettings() {
  if (!settings_ || !have_snapshot_ || restoring_) return;
  settings_->setValue(QStringLiteral("overlay/geometry"), saved_geometry_);
  settings_->setValue(QStringLiteral("overlay/opacity"), saved_opacity_);
  if (overlay_) {
    if (auto* screen = QGuiApplication::screenAt(overlay_->geometry().center()))
      settings_->setValue(QStringLiteral("overlay/displayName"), screen->name());
  }
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
  const QPoint topLeft(bounds.center().x() - size.width() / 2, bounds.center().y() - size.height() / 2);
  overlay_->setScreen(screen);
  overlay_->setGeometry(QRect(topLeft, size));
  keepOnScreen(); scheduleSave(); return true;
}
bool OverlayController::resizeOverlay(int width, int height) {
  if (!overlay_ || width <= 0 || height <= 0) return false;
  overlay_->resize(width, height); keepOnScreen(); scheduleSave(); return true;
}
bool OverlayController::eventFilter(QObject* watched, QEvent* event) {
  if (watched == control_ && (event->type() == QEvent::Hide || event->type() == QEvent::Close ||
      (event->type() == QEvent::WindowStateChange && control_->windowState() == Qt::WindowMinimized))) {
    saveSettings(); setClickThrough(false);
    if (event->type() == QEvent::Close && overlay_) overlay_->hide();
  }
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
