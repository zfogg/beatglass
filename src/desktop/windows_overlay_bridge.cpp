#include "windows_overlay_bridge.hpp"
#include "platform/windows_overlay.hpp"
WindowsOverlay::WindowsOverlay(QQuickWindow* overlay, QWindow* control, QObject* parent, QSettings* settings)
  : QObject(parent), controller_(std::make_unique<beatglass::windows::OverlayController>(overlay, control, nullptr, settings)) {
  connect(controller_.get(), &beatglass::windows::OverlayController::clickThroughChanged, this, &WindowsOverlay::clickThroughChanged);
  connect(controller_.get(), &beatglass::windows::OverlayController::opacityChanged, this, &WindowsOverlay::opacityChanged);
}
WindowsOverlay::~WindowsOverlay() = default;
bool WindowsOverlay::clickThrough() const { return controller_->clickThrough(); }
bool WindowsOverlay::setClickThrough(bool enabled) { return controller_->setClickThrough(enabled); }
void WindowsOverlay::setOpacity(qreal value) { controller_->setOpacity(value); }
qreal WindowsOverlay::opacity() const { return controller_->opacity(); }
void WindowsOverlay::keepOnScreen() { controller_->keepOnScreen(); }
bool WindowsOverlay::beginDrag() { return controller_->beginDrag(); }
void WindowsOverlay::dragToCursor() { controller_->dragToCursor(); }
void WindowsOverlay::endDrag() { controller_->endDrag(); }
