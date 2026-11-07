#include "windows_overlay_bridge.hpp"
#include "platform/windows_overlay.hpp"
WindowsOverlay::WindowsOverlay(QQuickWindow* overlay, QWindow* control, QObject* parent)
  : QObject(parent), controller_(std::make_unique<beatglass::windows::OverlayController>(overlay, control)) {
  connect(controller_.get(), &beatglass::windows::OverlayController::clickThroughChanged, this, &WindowsOverlay::clickThroughChanged);
}
WindowsOverlay::~WindowsOverlay() = default;
bool WindowsOverlay::clickThrough() const { return controller_->clickThrough(); }
bool WindowsOverlay::setClickThrough(bool enabled) { return controller_->setClickThrough(enabled); }
void WindowsOverlay::setOpacity(qreal value) { controller_->setOpacity(value); }
void WindowsOverlay::keepOnScreen() { controller_->keepOnScreen(); }
