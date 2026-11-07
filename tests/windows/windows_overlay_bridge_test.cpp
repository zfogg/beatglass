#include "windows_overlay_bridge.hpp"
#include <QQuickWindow>
#include <QSignalSpy>
#include <QGuiApplication>
#include <QScreen>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>
class WindowsOverlayBridgeTest : public QObject {
 Q_OBJECT
private slots:
 void preferencesSurviveBridgeRecreation() {
   QTemporaryDir directory; QVERIFY(directory.isValid());
   const auto path = directory.filePath("settings.ini");
   const auto position = QGuiApplication::primaryScreen()->availableGeometry().topLeft() + QPoint(120, 140);
   {
     QSettings settings(path, QSettings::IniFormat);
     QQuickWindow control, overlay; overlay.resize(240, 110); control.show();
     WindowsOverlay bridge(&overlay, &control, nullptr, &settings);
     overlay.setPosition(position); bridge.setOpacity(0.4);
     QVERIFY(bridge.setClickThrough(true)); settings.sync(); QCOMPARE(settings.status(), QSettings::NoError);
   }
   {
     QSettings settings(path, QSettings::IniFormat);
     QQuickWindow control, overlay; overlay.resize(240, 110);
     WindowsOverlay bridge(&overlay, &control, nullptr, &settings);
     QCOMPARE(overlay.position(), position); QVERIFY(qAbs(bridge.opacity() - 0.4) < 0.02);
     QVERIFY(!bridge.clickThrough()); QVERIFY(!overlay.isVisible());
   }
 }
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
