#include "macos_overlay.hpp"
#include <QGuiApplication>
#include <QQuickWindow>
#include <QtTest>
#import <AppKit/AppKit.h>
using namespace beatglass::macos;

class NativeOverlayTest final : public QObject {
  Q_OBJECT
private slots:
  void windowPropertiesAndControlEscape() {
    if (QGuiApplication::platformName() != QStringLiteral("cocoa")) QSKIP("Requires a logged-in Cocoa desktop");
    QQuickWindow overlay; QWindow control;
    control.setGeometry(30, 30, 250, 120); overlay.setGeometry(300, 80, 300, 140);
    control.show();
    OverlayController controller(makeOverlayNative());
    QVERIFY(controller.attach(&overlay, &control));
    QVERIFY(controller.showOverlay());
    NSView* view = reinterpret_cast<NSView*>(overlay.winId());
    NSWindow* window = view.window;
    QVERIFY(window != nil);
    QVERIFY(window.level >= NSFloatingWindowLevel);
    QVERIFY((window.collectionBehavior & NSWindowCollectionBehaviorCanJoinAllSpaces) != 0);
    QVERIFY((window.collectionBehavior & NSWindowCollectionBehaviorFullScreenAuxiliary) != 0);
    QVERIFY(controller.setClickThrough(true));
    QVERIFY(window.ignoresMouseEvents);
    control.hide();
    QTRY_VERIFY(!controller.clickThrough());
    QVERIFY(!window.ignoresMouseEvents);
    controller.hideOverlay();
    QVERIFY(!overlay.isVisible());
  }
};
QTEST_MAIN(NativeOverlayTest)
#include "native_overlay_test.moc"
