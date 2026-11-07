#include "windows_dj_bridge.hpp"
#include <QtTest>
#include <QThread>
#include <atomic>
#include <chrono>
#include <utility>
#include <stdexcept>
#include <cmath>
#include <vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>

class DjBridgeTest : public QObject {
  Q_OBJECT
private slots:
  void stopStartCannotSkipClear() {
    beatglass::windows::DjBridge bridge(nullptr, [] { return beatglass::windows::EndpointResult{}; });
    QSignalSpy updates(&bridge, &beatglass::windows::DjBridge::presentationUpdate);
    beatglass::PresentationGate gate;
    bool deliveredOnOwner = true;
    connect(&bridge, &beatglass::windows::DjBridge::presentationUpdate, &bridge, [&](beatglass::PresentationUpdate update) {
      deliveredOnOwner = deliveredOnOwner && QThread::currentThread() == bridge.thread();
      gate.apply(std::move(update));
    });
    bridge.start();
    QTRY_COMPARE(bridge.status(), QStringLiteral("Waiting for default output"));
    QVERIFY(bridge.active());
    bridge.stop();
    bridge.start();
    QTRY_VERIFY(!updates.isEmpty());
    auto first = qvariant_cast<beatglass::PresentationUpdate>(updates.takeFirst().at(0));
    QVERIFY(!first.snapshot);
    QVERIFY(bridge.active());
    bridge.stop();
    QTRY_VERIFY(!updates.isEmpty());
    auto second = qvariant_cast<beatglass::PresentationUpdate>(updates.takeLast().at(0));
    QVERIFY(!second.snapshot);
    QVERIFY(second.revision > first.revision);
    QVERIFY(deliveredOnOwner);
    QVERIFY(!gate.current());
    QVERIFY(!gate.apply(first));
    QVERIFY(!bridge.active());
  }
  void discoveryErrorIsDelivered() {
    beatglass::windows::DjBridge bridge(nullptr, [] { return beatglass::windows::EndpointResult{{}, "enumeration failed"}; });
    bridge.start();
    QTRY_COMPARE(bridge.status(), QStringLiteral("enumeration failed"));
    bridge.stop();
  }
  void destructionJoinsBlockedDiscovery() {
    std::mutex mutex;
    std::condition_variable wake;
    bool release = false;
    std::atomic<bool> entered{false};
    auto bridge = std::make_unique<beatglass::windows::DjBridge>(nullptr, [&] {
      std::unique_lock lock(mutex);
      entered.store(true);
      wake.wait(lock, [&] { return release; });
      return beatglass::windows::EndpointResult{};
    });
    bridge->start();
    QTRY_VERIFY(entered.load());
    std::thread unblock([&] {
      std::this_thread::sleep_for(std::chrono::milliseconds(30));
      { std::lock_guard lock(mutex); release = true; }
      wake.notify_one();
    });
    bridge.reset();
    unblock.join();
    QVERIFY(entered.load());
  }
  void fatalFailureRemainsVisibleUntilStop() {
    beatglass::windows::DjBridge bridge(nullptr, []() -> beatglass::windows::EndpointResult { throw std::runtime_error("failure"); });
    bridge.start();
    QTRY_COMPARE(bridge.status(), QStringLiteral("Capture or presentation error"));
    QVERIFY(!bridge.active());
    QTest::qWait(200);
    QCOMPARE(bridge.status(), QStringLiteral("Capture or presentation error"));
    bridge.stop();
    QTRY_COMPARE(bridge.status(), QStringLiteral("Stopped"));
  }
  void generatedPlaybackReachesGuiAndStopClears() {
    if (!qEnvironmentVariableIsSet("BEATGLASS_NATIVE_DJ_TEST")) QSKIP("Set BEATGLASS_NATIVE_DJ_TEST to play a generated click track");
    const auto endpoints = beatglass::windows::enumerate_render_endpoints();
    QVERIFY(endpoints.error.empty());
    bool haveDefault = false;
    for (const auto& endpoint : endpoints.endpoints) haveDefault = haveDefault || endpoint.is_default;
    if (!haveDefault) QSKIP("No default render endpoint");
    beatglass::windows::DjBridge bridge;
    beatglass::PresentationGate gate;
    connect(&bridge, &beatglass::windows::DjBridge::presentationUpdate, &bridge,
            [&](beatglass::PresentationUpdate update) { gate.apply(std::move(update)); });
    bridge.start();
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
    QTRY_VERIFY_WITH_TIMEOUT(gate.current() && gate.current()->bpm, 20000);
    const auto prior = gate.current();
    QCOMPARE(waveOutReset(output), MMSYSERR_NOERROR);
    bridge.stop();
    QTRY_VERIFY_WITH_TIMEOUT(!gate.current(), 5000);
    QVERIFY(prior->bpm);
    QTest::qWait(200);
    QVERIFY(!gate.current());
  }
};
QTEST_GUILESS_MAIN(DjBridgeTest)
#include "dj_bridge_test.moc"
