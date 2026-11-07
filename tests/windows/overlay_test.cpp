#include "windows_overlay.hpp"
#include <QGuiApplication>
#include <QQuickWindow>
#include <QScreen>
#include <QtTest>

class OverlayTest : public QObject {
  Q_OBJECT
private slots:
  void flagsAndEscape() {
    QQuickWindow overlay;
    QWindow control;
    beatglass::windows::OverlayController adapter(&overlay, &control);
    QVERIFY(overlay.flags().testFlag(Qt::WindowStaysOnTopHint));
    QVERIFY(overlay.flags().testFlag(Qt::FramelessWindowHint));
    QCOMPARE(overlay.color(), QColor(Qt::transparent));
    QVERIFY(!adapter.setClickThrough(true));
    control.show();
    QVERIFY(adapter.setClickThrough(true));
    QVERIFY(overlay.flags().testFlag(Qt::WindowTransparentForInput));
    control.hide();
    QVERIFY(!adapter.clickThrough());
    QVERIFY(!overlay.flags().testFlag(Qt::WindowTransparentForInput));
    adapter.setOpacity(0.4);
    QVERIFY(qAbs(overlay.opacity() - 0.4) < 0.02);
  }
  void offscreenGeometryIsRecovered() {
    QQuickWindow overlay;
    QWindow control;
    beatglass::windows::OverlayController adapter(&overlay, &control);
    overlay.setGeometry(-30000, -30000, 300, 200);
    adapter.keepOnScreen();
    QVERIFY(QGuiApplication::primaryScreen()->availableGeometry().contains(overlay.geometry()));
  }
};
QTEST_MAIN(OverlayTest)
#include "overlay_test.moc"
