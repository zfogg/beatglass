#include "metadata.hpp"
#include "live_presentation.hpp"
#include "windows_dj_bridge.hpp"
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtTest>
#include <cmath>
#include <vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>

class DjUiTest : public QObject {
  Q_OBJECT
private slots:
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
