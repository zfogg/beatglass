#include "macos_overlay.hpp"
#include <QQuickWindow>
#import <AppKit/AppKit.h>
#include <memory>

namespace beatglass::macos {
namespace {
class AppKitOverlay final : public OverlayNative {
public:
  bool apply(QQuickWindow* window, bool clickThrough) override {
    if (!window) return false;
    NSView* view = reinterpret_cast<NSView*>(window->winId());
    NSWindow* native = view.window;
    if (!native) return false;
    native.level = NSFloatingWindowLevel;
    native.hidesOnDeactivate = NO;
    NSUInteger behavior = native.collectionBehavior;
    behavior &= ~(NSWindowCollectionBehaviorMoveToActiveSpace | NSWindowCollectionBehaviorFullScreenPrimary |
                  NSWindowCollectionBehaviorFullScreenNone);
    native.collectionBehavior = static_cast<NSWindowCollectionBehavior>(behavior | NSWindowCollectionBehaviorCanJoinAllSpaces |
                                NSWindowCollectionBehaviorFullScreenAuxiliary | NSWindowCollectionBehaviorIgnoresCycle);
    native.ignoresMouseEvents = clickThrough ? YES : NO;
    return native.ignoresMouseEvents == (clickThrough ? YES : NO);
  }
  void reset() noexcept override {}
};
}
std::unique_ptr<OverlayNative> makeOverlayNative() { return std::make_unique<AppKitOverlay>(); }
}
