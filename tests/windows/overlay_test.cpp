#include "windows_overlay.hpp"
#include <QGuiApplication>
#include <QQuickWindow>
#include <QScreen>
#include <QtTest>
#include <windows.h>

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
    overlay.show();
    QVERIFY(adapter.setClickThrough(true));
    QVERIFY(overlay.flags().testFlag(Qt::WindowTransparentForInput));
    QTRY_VERIFY((GetWindowLongPtrW(reinterpret_cast<HWND>(overlay.winId()), GWL_EXSTYLE) & WS_EX_TRANSPARENT) != 0);
    QVERIFY((GetWindowLongPtrW(reinterpret_cast<HWND>(overlay.winId()), GWL_EXSTYLE) & WS_EX_TOPMOST) != 0);
    control.hide();
    QVERIFY(!adapter.clickThrough());
    QVERIFY(!overlay.flags().testFlag(Qt::WindowTransparentForInput));
    QTRY_VERIFY((GetWindowLongPtrW(reinterpret_cast<HWND>(overlay.winId()), GWL_EXSTYLE) & WS_EX_TRANSPARENT) == 0);
    control.show();
    QVERIFY(adapter.setClickThrough(true));
    control.setWindowState(Qt::WindowMinimized);
    QTRY_VERIFY(!adapter.clickThrough());
    QVERIFY(!adapter.setClickThrough(true));
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
  void destructionUnlocksOverlay() {
    QQuickWindow overlay;
    QWindow control;
    control.show();
    {
      beatglass::windows::OverlayController adapter(&overlay, &control);
      QVERIFY(adapter.setClickThrough(true));
    }
    QVERIFY(!overlay.flags().testFlag(Qt::WindowTransparentForInput));
  }
};
QTEST_MAIN(OverlayTest)
#include "overlay_test.moc"
