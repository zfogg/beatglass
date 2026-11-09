#include "windows_overlay.hpp"
#include "windows_overlay_policy.hpp"
#include <QGuiApplication>
#include <QDebug>
#include <QQuickWindow>
#include <QScreen>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>
#include <windows.h>
#include <QMouseEvent>

class ClickWindow : public QQuickWindow {
public:
  int presses{};
protected:
  void mousePressEvent(QMouseEvent* event) override { ++presses; event->accept(); }
};

class OverlayTest : public QObject {
  Q_OBJECT
private slots:
  void clickThroughPolicyRequiresVisibleEscapePath() {
    using beatglass::windows::OverlayControlState;
    using beatglass::windows::canEnableClickThrough;
    QVERIFY(!canEnableClickThrough(false, false, OverlayControlState::visible));
    QVERIFY(!canEnableClickThrough(true, true, OverlayControlState::visible));
    QVERIFY(!canEnableClickThrough(true, false, OverlayControlState::hidden));
    QVERIFY(!canEnableClickThrough(true, false, OverlayControlState::minimized));
    QVERIFY(!canEnableClickThrough(true, false, OverlayControlState::unavailable));
    QVERIFY(canEnableClickThrough(true, false, OverlayControlState::visible));
  }
  void recoveredPositionIsPersistedImmediately() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath("overlay.ini");
    QPoint recovered;
    {
      QSettings settings(path, QSettings::IniFormat);
      QQuickWindow overlay;
      overlay.resize(320, 110);
      QWindow control;
      beatglass::windows::OverlayController controller(&overlay, &control, nullptr, &settings);
      overlay.setPosition(-30000, -30000);
      controller.keepOnScreen();
      recovered = overlay.position();
      QVERIFY(QGuiApplication::primaryScreen()->availableGeometry().contains(overlay.geometry()));
      settings.sync();
      QCOMPARE(settings.value(QStringLiteral("overlay/position")).toPoint(), recovered);
      QCOMPARE(settings.status(), QSettings::NoError);
    }
    {
      QSettings settings(path, QSettings::IniFormat);
      QQuickWindow overlay;
      overlay.resize(320, 110);
      QWindow control;
      beatglass::windows::OverlayController controller(&overlay, &control, nullptr, &settings);
      QCOMPARE(overlay.position(), recovered);
    }
  }
  void restoresSavedPositionAndOpacity() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath("overlay.ini");
    const auto position = QGuiApplication::primaryScreen()->availableGeometry().topLeft() + QPoint(120, 140);
    {
      QSettings settings(path, QSettings::IniFormat);
      QQuickWindow overlay;
      QWindow control;
      control.show();
      beatglass::windows::OverlayController controller(&overlay, &control, nullptr, &settings);
      overlay.setPosition(position);
      QVERIFY(controller.resizeOverlay(420, 180));
      QVERIFY(controller.setOpacity(0.4));
      QVERIFY(controller.setClickThrough(true));
      settings.sync();
      QCOMPARE(settings.status(), QSettings::NoError);
    }
    {
      QSettings settings(path, QSettings::IniFormat);
      QQuickWindow overlay;
      overlay.resize(320, 110);
      QWindow control;
      beatglass::windows::OverlayController controller(&overlay, &control, nullptr, &settings);
      QCOMPARE(overlay.position(), position);
      QCOMPARE(overlay.size(), QSize(420, 180));
      QVERIFY(qAbs(overlay.opacity() - 0.4) < 0.02);
      QVERIFY(!controller.clickThrough());
      QVERIFY(!overlay.isVisible());
    }
  }
  void flagsAndEscape() {
    QQuickWindow overlay;
    QWindow control;
    beatglass::windows::OverlayController adapter(&overlay, &control);
    QSignalSpy changes(&adapter, &beatglass::windows::OverlayController::clickThroughChanged);
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
    QCOMPARE(changes.size(), 2);
    QCOMPARE(changes.last().first().toBool(), false);
    QVERIFY(!overlay.flags().testFlag(Qt::WindowTransparentForInput));
    QTRY_VERIFY((GetWindowLongPtrW(reinterpret_cast<HWND>(overlay.winId()), GWL_EXSTYLE) & WS_EX_TRANSPARENT) == 0);
    control.show();
    QVERIFY(adapter.setClickThrough(true));
    control.setWindowState(Qt::WindowMinimized);
    QTRY_VERIFY(!adapter.clickThrough());
    QVERIFY(!adapter.setClickThrough(true));
    adapter.setOpacity(0.4);
    QVERIFY(qAbs(overlay.opacity() - 0.4) < 0.02);
    adapter.setOpacity(0.0);
    QVERIFY(qAbs(overlay.opacity() - 0.2) < 0.02);
  }
  void clickThroughRequiresVisibleOverlay() {
    QQuickWindow overlay;
    QWindow control;
    control.show();
    beatglass::windows::OverlayController adapter(&overlay, &control);
    QVERIFY(!overlay.isVisible());
    QVERIFY(!adapter.setClickThrough(true));
    overlay.show();
    QVERIFY(adapter.setClickThrough(true));
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
  void hidingOverlayUnlocksInput() {
    QQuickWindow overlay;
    QWindow control;
    control.show();
    beatglass::windows::OverlayController adapter(&overlay, &control);
    overlay.show();
    QVERIFY(adapter.setClickThrough(true));
    overlay.hide();
    QTRY_VERIFY(!adapter.clickThrough());
    QVERIFY(!overlay.flags().testFlag(Qt::WindowTransparentForInput));
    overlay.show();
    QVERIFY(adapter.setClickThrough(true));
  }
  void lifecycleAndResizeAreSafe() {
    QQuickWindow overlay;
    QWindow control;
    beatglass::windows::OverlayController adapter(&overlay, &control);
    QVERIFY(adapter.available());
    QVERIFY(!overlay.isVisible());
    QVERIFY(adapter.showOverlay());
    QVERIFY(overlay.isVisible());
    QVERIFY(adapter.resizeOverlay(420, 180));
    QCOMPARE(overlay.size(), QSize(420, 180));
    QVERIFY(!adapter.resizeOverlay(0, 180));
    adapter.hideOverlay();
    QVERIFY(!overlay.isVisible());
  }
  void closingControlHidesOrphanedOverlay() {
    QQuickWindow overlay;
    QWindow control;
    beatglass::windows::OverlayController adapter(&overlay, &control);
    control.show();
    QVERIFY(adapter.showOverlay());
    QVERIFY(overlay.isVisible());
    QVERIFY(control.close());
    QTRY_VERIFY(!overlay.isVisible());
    QVERIFY(!adapter.clickThrough());
  }
  void controlCannotBeOverlay() {
    QQuickWindow window;
    window.show();
    beatglass::windows::OverlayController adapter(&window, &window);
    QVERIFY(!adapter.setClickThrough(true));
    QVERIFY(!adapter.clickThrough());
    QVERIFY(!window.flags().testFlag(Qt::WindowTransparentForInput));
  }

  void displaySelectionUsesCurrentScreens() {
    QQuickWindow overlay;
    QWindow control;
    beatglass::windows::OverlayController adapter(&overlay, &control);
    const auto screens = QGuiApplication::screens();
    QCOMPARE(adapter.screenNames().size(), screens.size());
    if (screens.isEmpty()) {
      QCOMPARE(adapter.screenIndex(), -1);
      QVERIFY(!adapter.moveToScreen(0));
      return;
    }
    QVERIFY(adapter.moveToScreen(0));
    QCOMPARE(adapter.screenIndex(), 0);
    QVERIFY(screens.at(0)->availableGeometry().contains(overlay.geometry().center()));
  }
  void displaySelectionPublishesRecoverySignals() {
    const auto screens = QGuiApplication::screens();
    if (screens.isEmpty()) QSKIP("No active display is available");
    QQuickWindow overlay;
    QWindow control;
    beatglass::windows::OverlayController adapter(&overlay, &control);
    QSignalSpy geometry(&adapter, &beatglass::windows::OverlayController::geometryChanged);
    QSignalSpy screensChanged(&adapter, &beatglass::windows::OverlayController::screensChanged);
    QVERIFY(adapter.moveToScreen(0));
    QVERIFY(geometry.count() > 0);
    QVERIFY(screensChanged.count() > 0);
    QVERIFY(screens.at(0)->availableGeometry().contains(overlay.geometry().center()));
  }
  void restoresSavedDisplayWhenNameMatches() {
    const auto screens = QGuiApplication::screens();
    if (screens.isEmpty() || screens.at(0)->name().isEmpty()) QSKIP("No named display is available");
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath("overlay.ini");
    {
      QSettings settings(path, QSettings::IniFormat);
      QQuickWindow overlay;
      QWindow control;
      beatglass::windows::OverlayController adapter(&overlay, &control, nullptr, &settings);
      QVERIFY(adapter.moveToScreen(0));
      settings.sync();
    }
    QSettings restored(path, QSettings::IniFormat);
    QCOMPARE(restored.value(QStringLiteral("overlay/displayName")).toString(), screens.at(0)->name());
  }
  void nativePointerPassThrough() {
    if (!qEnvironmentVariableIsSet("BEATGLASS_NATIVE_OVERLAY_TEST")) QSKIP("Set BEATGLASS_NATIVE_OVERLAY_TEST to run the interactive pointer test");
    struct RestoreCursor { POINT position{}; RestoreCursor() { GetCursorPos(&position); } ~RestoreCursor() { SetCursorPos(position.x, position.y); } } restore;
    ClickWindow background;
    ClickWindow overlay;
    QWindow control;
    const auto bounds = QGuiApplication::primaryScreen()->availableGeometry();
    const QRect area(bounds.left() + 120, bounds.top() + 120, 320, 220);
    background.setGeometry(area);
    background.setColor(Qt::darkBlue);
    control.setGeometry(bounds.left() + 10, bounds.top() + 10, 80, 60);
    overlay.setGeometry(area);
    control.show();
    background.show();
    beatglass::windows::OverlayController adapter(&overlay, &control);
    overlay.show();
    const auto hwnd = reinterpret_cast<HWND>(overlay.winId());
    QVERIFY(SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE));
    QTest::qWait(200);
    auto click = [&] {
      const auto point = overlay.mapToGlobal(QPoint(160, 110));
      // SetCursorPos uses physical pixels; convert the Qt logical position for the target screen.
      POINT native{point.x(), point.y()};
      QVERIFY(LogicalToPhysicalPointForPerMonitorDPI(hwnd, &native));
      QVERIFY(SetCursorPos(native.x, native.y));
      INPUT input[2]{};
      input[0].type = INPUT_MOUSE; input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
      input[1].type = INPUT_MOUSE; input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
      QCOMPARE(SendInput(2, input, sizeof(INPUT)), UINT(2));
    };
    click();
    QTRY_COMPARE(overlay.presses, 1);
    QCOMPARE(background.presses, 0);
    QVERIFY(adapter.setClickThrough(true));
    QTest::qWait(100);
    click();
    QTRY_COMPARE(background.presses, 1);
    QCOMPARE(overlay.presses, 1);
    control.hide();
    QTRY_VERIFY(!adapter.clickThrough());
    click();
    QTRY_COMPARE(overlay.presses, 2);
    control.show();
    background.showFullScreen();
    QTest::qWait(300);
    QVERIFY(SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE));
    QVERIFY(adapter.setClickThrough(true));
    QTest::qWait(100);
    click();
    QTRY_COMPARE(background.presses, 2);
    QVERIFY(adapter.setClickThrough(false));
    click();
    QTRY_COMPARE(overlay.presses, 3);
  }
  void nativeMultiMonitorPassThrough() {
    if (!qEnvironmentVariableIsSet("BEATGLASS_NATIVE_OVERLAY_TEST")) QSKIP("Set BEATGLASS_NATIVE_OVERLAY_TEST to run the interactive pointer test");
    const auto screens = QGuiApplication::screens();
    if (screens.size() < 2) QSKIP("At least two active displays are required");
    struct RestoreCursor { POINT position{}; RestoreCursor() { GetCursorPos(&position); } ~RestoreCursor() { SetCursorPos(position.x, position.y); } } restore;
    ClickWindow background, overlay;
    QWindow control;
    control.setGeometry(QGuiApplication::primaryScreen()->availableGeometry().topLeft().x() + 10,
                        QGuiApplication::primaryScreen()->availableGeometry().topLeft().y() + 10, 80, 60);
    control.show();
    beatglass::windows::OverlayController adapter(&overlay, &control);
    background.show();
    overlay.show();
    QVERIFY(adapter.setClickThrough(true));
    int expected = 0;
    for (auto* screen : screens) {
      qInfo() << "Display" << screen->name() << "logical geometry" << screen->geometry() << "scale" << screen->devicePixelRatio();
      const auto origin = screen->availableGeometry().topLeft();
      background.setScreen(screen);
      overlay.setScreen(screen);
      const QRect area(origin.x() + 120, origin.y() + 120, 320, 220);
      background.setGeometry(area);
      overlay.setGeometry(area);
      const auto hwnd = reinterpret_cast<HWND>(overlay.winId());
      QVERIFY(SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE));
      QTest::qWait(200);
      const auto point = overlay.mapToGlobal(QPoint(160, 110));
      POINT native{point.x(), point.y()};
      QVERIFY(LogicalToPhysicalPointForPerMonitorDPI(hwnd, &native));
      QVERIFY(SetCursorPos(native.x, native.y));
      INPUT input[2]{};
      input[0].type = INPUT_MOUSE; input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
      input[1].type = INPUT_MOUSE; input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
      QCOMPARE(SendInput(2, input, sizeof(INPUT)), UINT(2));
      ++expected;
      QTRY_COMPARE(background.presses, expected);
    }
  }
};
QTEST_MAIN(OverlayTest)
#include "overlay_test.moc"
