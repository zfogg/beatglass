#include "metadata.hpp"
#include "live_presentation.hpp"
#include "windows_dj_bridge.hpp"
#include "windows_overlay.hpp"
#include <QQuickWindow>
#include <QScreen>
#include <QSettings>
#include <QTemporaryDir>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtTest>
#include <cmath>
#include <vector>
#include <thread>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>

class DjUiTest : public QObject {
  Q_OBJECT
private slots:
  void overlaySettingsReachQmlAfterRestart() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath("settings.ini");
    const auto position = QGuiApplication::primaryScreen()->availableGeometry().topLeft() + QPoint(150, 160);
    Metadata metadata;
    LivePresentation presentation;
    for (int run = 0; run != 2; ++run) {
      QSettings settings(path, QSettings::IniFormat);
      QQmlApplicationEngine engine;
      engine.rootContext()->setContextProperty("metadata", &metadata);
      engine.rootContext()->setContextProperty("livePresentation", &presentation);
      engine.load(QUrl("qrc:/Main.qml"));
      QVERIFY(!engine.rootObjects().isEmpty());
      auto* root = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
      QVERIFY(root);
      auto* overlay = root->findChild<QQuickWindow*>("overlayWindow");
      auto* slider = root->findChild<QObject*>("overlayOpacity");
      QVERIFY(overlay); QVERIFY(slider);
      auto* controller = new beatglass::windows::OverlayController(overlay, root, root, &settings);
      QVERIFY(root->setProperty("overlayControl", QVariant::fromValue(static_cast<QObject*>(controller))));
      if (run == 0) {
        overlay->setPosition(position);
        controller->setOpacity(0.4);
        settings.sync();
        QCOMPARE(settings.status(), QSettings::NoError);
      } else {
        QCOMPARE(overlay->position(), position);
        QVERIFY(qAbs(slider->property("value").toReal() - 0.4) < 0.02);
        QVERIFY(!controller->clickThrough());
        QVERIFY(!overlay->isVisible());
      }
    }
  }
  void nativeCrossDisplayDrag() {
    if (!qEnvironmentVariableIsSet("BEATGLASS_NATIVE_OVERLAY_TEST")) QSKIP("Set BEATGLASS_NATIVE_OVERLAY_TEST for native pointer input");
    struct RestoreCursor { POINT position{}; RestoreCursor() { GetCursorPos(&position); } ~RestoreCursor() { SetCursorPos(position.x, position.y); } } restore;
    Metadata metadata;
    LivePresentation presentation;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("metadata", &metadata);
    engine.rootContext()->setContextProperty("livePresentation", &presentation);
    engine.load(QUrl("qrc:/Main.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    auto* root = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    QVERIFY(root);
    auto* overlay = root->findChild<QQuickWindow*>("overlayWindow");
    QVERIFY(overlay);
    beatglass::windows::OverlayController controller(overlay, root);
    QVERIFY(root->setProperty("overlayControl", QVariant::fromValue(static_cast<QObject*>(&controller))));
    overlay->setPosition(QGuiApplication::primaryScreen()->availableGeometry().topLeft() + QPoint(20, 300));
    overlay->show();
    QTRY_VERIFY(overlay->isExposed());
    QTest::qWait(200);
    const auto hwnd = reinterpret_cast<HWND>(overlay->winId());
    const auto source = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitor{}; monitor.cbSize = sizeof(monitor);
    QVERIFY(GetMonitorInfoW(source, &monitor));
    RECT rect{};
    QVERIFY(GetWindowRect(hwnd, &rect));
    POINT start{rect.left + 30, rect.top + 30};
    POINT destination{monitor.rcMonitor.left - 500, start.y};
    const auto target = MonitorFromPoint(destination, MONITOR_DEFAULTTONULL);
    if (!target || target == source) QSKIP("A display immediately to the left of the primary is required");
    QVERIFY(SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE));
    SetForegroundWindow(hwnd);
    std::jthread pointer([start, destination] {
      SetCursorPos(start.x, start.y);
      INPUT input{}; input.type = INPUT_MOUSE; input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
      SendInput(1, &input, sizeof(INPUT));
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      for (int step = 1; step <= 30; ++step) {
        SetCursorPos(start.x + (destination.x - start.x) * step / 30, start.y);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
      }
      input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
      SendInput(1, &input, sizeof(INPUT));
    });
    QTest::qWait(2000);
    pointer.join();
    QVERIFY(GetWindowRect(hwnd, &rect));
    qInfo() << "Cross-display final bounds" << rect.left << rect.top << rect.right << rect.bottom << "destination" << destination.x << destination.y;
    QCOMPARE(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), target);
    QVERIFY(GetWindowRect(hwnd, &rect));
    QVERIFY2(PtInRect(&rect, destination), "The overlay must remain beneath the pointer after crossing the DPI boundary");
  }
  void shutdownDuringMonitoringAndClickThrough() {
    Metadata metadata;
    LivePresentation presentation;
    beatglass::windows::DjBridge bridge;
    connect(&bridge, &beatglass::windows::DjBridge::presentationUpdate, &presentation,
            [&presentation](beatglass::PresentationUpdate update) { presentation.apply(std::move(update)); });
    QPointer<QQuickWindow> rootPointer;
    QPointer<QQuickWindow> overlayPointer;
    {
      QQmlApplicationEngine engine;
      engine.rootContext()->setContextProperty("metadata", &metadata);
      engine.rootContext()->setContextProperty("livePresentation", &presentation);
      engine.load(QUrl("qrc:/Main.qml"));
      QVERIFY(!engine.rootObjects().isEmpty());
      auto* root = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
      QVERIFY(root);
      auto* overlay = root->findChild<QQuickWindow*>("overlayWindow");
      QVERIFY(overlay);
      rootPointer = root;
      overlayPointer = overlay;
      auto* controller = new beatglass::windows::OverlayController(overlay, root, root);
      QVERIFY(root->setProperty("overlayControl", QVariant::fromValue(static_cast<QObject*>(controller))));
      QVERIFY(root->setProperty("djControl", QVariant::fromValue(static_cast<QObject*>(&bridge))));
      overlay->show();
      QVERIFY(controller->setClickThrough(true));
      bridge.start();
      QVERIFY(bridge.active());
      root->close();
      QTRY_VERIFY(!controller->clickThrough());
    }
    QVERIFY(rootPointer.isNull());
    QVERIFY(overlayPointer.isNull());
    bridge.stop();
    QTRY_VERIFY(!bridge.active());
  }
  void nativeOverlayDrag() {
    if (!qEnvironmentVariableIsSet("BEATGLASS_NATIVE_OVERLAY_TEST")) QSKIP("Set BEATGLASS_NATIVE_OVERLAY_TEST for native pointer input");
    struct RestoreCursor { POINT position{}; RestoreCursor() { GetCursorPos(&position); } ~RestoreCursor() { SetCursorPos(position.x, position.y); } } restore;
    Metadata metadata;
    LivePresentation presentation;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("metadata", &metadata);
    engine.rootContext()->setContextProperty("livePresentation", &presentation);
    engine.load(QUrl("qrc:/Main.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    auto* root = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    QVERIFY(root);
    auto* overlay = root->findChild<QQuickWindow*>("overlayWindow");
    QVERIFY(overlay);
    beatglass::windows::OverlayController controller(overlay, root);
    QVERIFY(root->setProperty("overlayControl", QVariant::fromValue(static_cast<QObject*>(&controller))));
    auto* toggle = root->findChild<QObject*>("toggleOverlay");
    QVERIFY(toggle);
    QVERIFY(QMetaObject::invokeMethod(toggle, "clicked"));
    QTRY_VERIFY(overlay->isVisible());
    QTest::qWait(200);
    for (auto* screen : QGuiApplication::screens()) {
      const auto hwnd = reinterpret_cast<HWND>(overlay->winId());
      qInfo() << "Testing native overlay drag on" << screen->name() << "scale" << screen->devicePixelRatio() << "geometry" << screen->geometry();
      MONITORINFO monitor{}; monitor.cbSize = sizeof(monitor);
      POINT screenOrigin{screen->geometry().x() + 1, screen->geometry().y() + 1};
      const auto expectedMonitor = MonitorFromPoint(screenOrigin, MONITOR_DEFAULTTONULL);
      QVERIFY(expectedMonitor);
      QVERIFY(GetMonitorInfoW(expectedMonitor, &monitor));
      QVERIFY(SetWindowPos(hwnd, HWND_TOPMOST, monitor.rcWork.left + 100, monitor.rcWork.top + 100, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE));
      QTRY_COMPARE(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), expectedMonitor);
      QTest::qWait(200);
      const auto before = overlay->position();
      SetForegroundWindow(hwnd);
      RECT rect{};
      QVERIFY(GetWindowRect(hwnd, &rect));
      qInfo() << "Native monitor bounds" << monitor.rcMonitor.left << monitor.rcMonitor.top << monitor.rcMonitor.right << monitor.rcMonitor.bottom;
      POINT native{rect.left + 30, rect.top + 30};
      std::jthread pointer([native] {
        SetCursorPos(native.x, native.y);
        INPUT input{}; input.type = INPUT_MOUSE; input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &input, sizeof(INPUT));
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        for (int step = 1; step <= 10; ++step) {
          SetCursorPos(native.x + step * 10, native.y + step * 6);
          std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &input, sizeof(INPUT));
      });
      QTRY_VERIFY_WITH_TIMEOUT(overlay->position().x() > before.x() + 20 && overlay->position().y() > before.y() + 10, 2000);
      pointer.join();
    }
  }
  void overlayControlsUseRealController() {
    Metadata metadata;
    LivePresentation presentation;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("metadata", &metadata);
    engine.rootContext()->setContextProperty("livePresentation", &presentation);
    engine.load(QUrl("qrc:/Main.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    auto* root = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    QVERIFY(root);
    auto* overlay = root->findChild<QQuickWindow*>("overlayWindow");
    QVERIFY(overlay);
    beatglass::windows::OverlayController controller(overlay, root);
    QVERIFY(root->setProperty("overlayControl", QVariant::fromValue(static_cast<QObject*>(&controller))));
    auto* navigation = root->findChild<QObject*>("navigation");
    QVERIFY(navigation); QVERIFY(navigation->setProperty("currentIndex", 1));
    auto* toggle = root->findChild<QObject*>("toggleOverlay");
    auto* lock = root->findChild<QObject*>("toggleClickThrough");
    auto* recover = root->findChild<QObject*>("recoverOverlay");
    auto* opacity = root->findChild<QObject*>("overlayOpacity");
    QVERIFY(toggle); QVERIFY(lock); QVERIFY(recover); QVERIFY(opacity);
    QVERIFY(QMetaObject::invokeMethod(toggle, "clicked"));
    QTRY_VERIFY(overlay->isVisible());
    QVERIFY(QMetaObject::invokeMethod(lock, "clicked"));
    QTRY_VERIFY(controller.clickThrough());
    QVERIFY(QMetaObject::invokeMethod(lock, "clicked"));
    QTRY_VERIFY(!controller.clickThrough());
    QVERIFY(opacity->setProperty("value", 0.4));
    QVERIFY(QMetaObject::invokeMethod(opacity, "moved"));
    QTRY_VERIFY(qAbs(overlay->opacity() - 0.4) < 0.02);
    overlay->setPosition(-30000, -30000);
    QVERIFY(QMetaObject::invokeMethod(recover, "clicked"));
    QVERIFY(QGuiApplication::primaryScreen()->availableGeometry().contains(overlay->geometry()));
    QVERIFY(QMetaObject::invokeMethod(lock, "clicked"));
    QVERIFY(controller.clickThrough());
    QVERIFY(QMetaObject::invokeMethod(toggle, "clicked"));
    QTRY_VERIFY(!overlay->isVisible());
    QVERIFY(!controller.clickThrough());
  }
  void controlsUseRealBridge() {
    Metadata metadata;
    LivePresentation presentation;
    beatglass::windows::DjBridge bridge;
    connect(&bridge, &beatglass::windows::DjBridge::presentationUpdate, &presentation,
            [&presentation](beatglass::PresentationUpdate update) { presentation.apply(std::move(update)); });
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("metadata", &metadata);
    engine.rootContext()->setContextProperty("livePresentation", &presentation);
    engine.load(QUrl("qrc:/Main.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    auto* root = engine.rootObjects().first();
    QVERIFY(root->setProperty("djControl", QVariant::fromValue(static_cast<QObject*>(&bridge))));
    auto* navigation = root->findChild<QObject*>("navigation");
    auto* page = root->findChild<QObject*>("djPage");
    QVERIFY(navigation); QVERIFY(page);
    QVERIFY(navigation->setProperty("currentIndex", 1));
    QTRY_VERIFY(page->property("visible").toBool());
    auto* start = root->findChild<QObject*>("startMonitoring");
    auto* stop = root->findChild<QObject*>("stopMonitoring");
    QVERIFY(start);
    QVERIFY(stop);
    QVERIFY(QMetaObject::invokeMethod(start, "clicked"));
    QTRY_VERIFY(bridge.active());
    QVERIFY(QMetaObject::invokeMethod(stop, "clicked"));
    QTRY_VERIFY(!bridge.active());
  }
  void generatedOutputUpdatesAndClearsQmlModel() {
    if (!qEnvironmentVariableIsSet("BEATGLASS_NATIVE_DJ_TEST")) QSKIP("Set BEATGLASS_NATIVE_DJ_TEST to play a generated click track");
    Metadata metadata;
    LivePresentation presentation;
    beatglass::windows::DjBridge bridge;
    connect(&bridge, &beatglass::windows::DjBridge::presentationUpdate, &presentation,
            [&presentation](beatglass::PresentationUpdate update) { presentation.apply(std::move(update)); });
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("metadata", &metadata);
    engine.rootContext()->setContextProperty("livePresentation", &presentation);
    engine.load(QUrl("qrc:/Main.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    auto* root = engine.rootObjects().first();
    QVERIFY(root->setProperty("djControl", QVariant::fromValue(static_cast<QObject*>(&bridge))));
    auto* navigation = root->findChild<QObject*>("navigation");
    auto* page = root->findChild<QObject*>("djPage");
    QVERIFY(navigation); QVERIFY(page);
    QVERIFY(navigation->setProperty("currentIndex", 1));
    QTRY_VERIFY(page->property("visible").toBool());
    auto* start = root->findChild<QObject*>("startMonitoring");
    auto* stop = root->findChild<QObject*>("stopMonitoring");
    auto* estimates = root->findChild<QObject*>("liveEstimates");
    QVERIFY(start); QVERIFY(stop);
    QVERIFY(estimates);
    QVERIFY(QMetaObject::invokeMethod(start, "clicked"));
    QTRY_COMPARE_WITH_TIMEOUT(bridge.status(), QStringLiteral("Monitoring default output"), 5000);
    WAVEFORMATEX format{};
    format.wFormatTag = WAVE_FORMAT_PCM; format.nChannels = 1; format.nSamplesPerSec = 44100;
    format.wBitsPerSample = 16; format.nBlockAlign = 2; format.nAvgBytesPerSec = 88200;
    std::vector<short> pcm(44100 * 20);
    for (std::size_t i = 0; i < pcm.size(); ++i) {
      const auto phase = i % 22050;
      if (phase < 1000) pcm[i] = static_cast<short>(10000.0 * std::exp(-static_cast<double>(phase) / 100.0) * std::sin(phase * 0.4));
    }
    HWAVEOUT output{};
    QCOMPARE(waveOutOpen(&output, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL), MMSYSERR_NOERROR);
    struct Output { HWAVEOUT handle; WAVEHDR header{}; bool prepared{}; ~Output() { waveOutReset(handle); if (prepared) waveOutUnprepareHeader(handle, &header, sizeof(header)); waveOutClose(handle); } } playback{output};
    playback.header.lpData = reinterpret_cast<LPSTR>(pcm.data());
    playback.header.dwBufferLength = static_cast<DWORD>(pcm.size() * sizeof(short));
    QCOMPARE(waveOutPrepareHeader(output, &playback.header, sizeof(playback.header)), MMSYSERR_NOERROR);
    playback.prepared = true;
    QCOMPARE(waveOutWrite(output, &playback.header, sizeof(playback.header)), MMSYSERR_NOERROR);
    QTRY_VERIFY_WITH_TIMEOUT(presentation.tempo() != QStringLiteral("\u2014"), 20000);
    QTRY_VERIFY(estimates->property("text").toString().contains("BPM: " + presentation.tempo()));
    QCOMPARE(waveOutReset(output), MMSYSERR_NOERROR);
    QVERIFY(QMetaObject::invokeMethod(stop, "clicked"));
    QTRY_COMPARE_WITH_TIMEOUT(presentation.tempo(), QStringLiteral("\u2014"), 5000);
    QTRY_VERIFY(estimates->property("text").toString().contains(QStringLiteral("BPM: \u2014")));
  }
};
QTEST_MAIN(DjUiTest)
#include "dj_ui_test.moc"
