#include "windows_overlay_bridge.hpp"
#include <QQuickWindow>
#include <QSignalSpy>
#include <QtTest>
class WindowsOverlayBridgeTest : public QObject {
 Q_OBJECT
private slots:
 void automaticUnlockReachesGui() {
   QQuickWindow control, overlay;
   control.show();
   WindowsOverlay bridge(&overlay, &control);
   QSignalSpy changes(&bridge, &WindowsOverlay::clickThroughChanged);
   bridge.setOpacity(0.65);
   QVERIFY(qAbs(overlay.opacity() - 0.65) < 0.01);
   QVERIFY(bridge.setClickThrough(true));
   QVERIFY(bridge.clickThrough());
   control.hide();
   QTRY_VERIFY(!bridge.clickThrough());
   QCOMPARE(changes.count(), 2);
 }
};
QTEST_MAIN(WindowsOverlayBridgeTest)
#include "windows_overlay_bridge_test.moc"
