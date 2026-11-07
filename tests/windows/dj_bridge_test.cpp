#include "windows_dj_bridge.hpp"
#include "beatglass/capture/file_reader.hpp"
#include <QtTest>
#include <QThread>
#include <QTemporaryDir>
#include <QFile>
#include <QDataStream>
#include <array>
#include <atomic>
#include <chrono>
#include <utility>
#include <stdexcept>
#include <limits>
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
  void fileReaderAndLiveRestartShareEpochs() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QFile file(directory.filePath("input.wav"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    QCOMPARE(stream.writeRawData("RIFF", 4), 4); stream << quint32(44);
    QCOMPARE(stream.writeRawData("WAVEfmt ", 8), 8); stream << quint32(16) << quint16(1) << quint16(1) << quint32(44100) << quint32(88200) << quint16(2) << quint16(16);
    QCOMPARE(stream.writeRawData("data", 4), 4); stream << quint32(8) << qint16(0) << qint16(1000) << qint16(-1000) << qint16(0);
    QCOMPARE(stream.status(), QDataStream::Ok);
    file.close();
    beatglass::SourceEpochAllocator epochs;
    const auto offlineEpoch = epochs.next();
    QVERIFY(offlineEpoch);
    beatglass::FileReader reader(std::filesystem::path(file.fileName().toStdWString()), *offlineEpoch);
    auto discovery = [] { return beatglass::windows::EndpointResult{{{"not-a-real-endpoint", "unavailable", true}}, {}}; };
    beatglass::windows::DjBridge bridge(epochs, nullptr, discovery);
    bridge.start();
    QTRY_VERIFY(bridge.status() != QStringLiteral("Stopped"));
    std::array<float, 4> output{};
    const auto block = reader.read(output);
    QCOMPARE(block.status, beatglass::FileReadStatus::data);
    QCOMPARE(block.block.epoch, *offlineEpoch);
    const auto afterLive = epochs.next();
    QVERIFY(afterLive);
    QVERIFY(*afterLive > *offlineEpoch + 1);
    bridge.stop();
    QTRY_COMPARE(bridge.status(), QStringLiteral("Stopped"));
    bridge.start();
    QTRY_VERIFY(bridge.status() != QStringLiteral("Stopped"));
    const auto afterRestart = epochs.next();
    QVERIFY(afterRestart);
    QVERIFY(*afterRestart > *afterLive + 1);
    bridge.stop();
  }
  void allocatorSurvivesStopRestartAndBridgeRecreation() {
    beatglass::SourceEpochAllocator epochs;
    const auto discovery = [] {
      return beatglass::windows::EndpointResult{{{"not-a-real-endpoint", "unavailable", true}}, {}};
    };
    auto first = std::make_unique<beatglass::windows::DjBridge>(epochs, nullptr, discovery);
    first->start();
    QTRY_VERIFY(first->status() != QStringLiteral("Stopped"));
    const auto offlineBeforeStop = epochs.next();
    QVERIFY(offlineBeforeStop);
    QVERIFY(*offlineBeforeStop > 1);
    first->stop();
    QTRY_COMPARE(first->status(), QStringLiteral("Stopped"));
    first->start();
    QTRY_VERIFY(first->status() != QStringLiteral("Stopped"));
    const auto offlineAfterRestart = epochs.next();
    QVERIFY(offlineAfterRestart);
    QVERIFY(*offlineAfterRestart > *offlineBeforeStop + 1);
    first.reset();
    auto second = std::make_unique<beatglass::windows::DjBridge>(epochs, nullptr, discovery);
    second->start();
    QTRY_VERIFY(second->status() != QStringLiteral("Stopped"));
    const auto offlineAfterRecreation = epochs.next();
    QVERIFY(offlineAfterRecreation);
    QVERIFY(*offlineAfterRecreation > *offlineAfterRestart + 1);
    second.reset();
    QVERIFY(epochs.next().value() > *offlineAfterRecreation);
  }
  void stopStartCannotSkipClear() {
    beatglass::SourceEpochAllocator epochs;
    beatglass::windows::DjBridge bridge(epochs, nullptr, [] { return beatglass::windows::EndpointResult{}; });
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
    beatglass::SourceEpochAllocator epochs;
    beatglass::windows::DjBridge bridge(epochs, nullptr, [] { return beatglass::windows::EndpointResult{{}, "enumeration failed"}; });
    bridge.start();
    QTRY_COMPARE(bridge.status(), QStringLiteral("enumeration failed"));
    bridge.stop();
  }
  void destructionJoinsBlockedDiscovery() {
    beatglass::SourceEpochAllocator epochs;
    std::mutex mutex;
    std::condition_variable wake;
    bool release = false;
    std::atomic<bool> entered{false};
    auto bridge = std::make_unique<beatglass::windows::DjBridge>(epochs, nullptr, [&] {
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
  void concurrentOfflineIssuanceAndLiveShutdown() {
    if (!qEnvironmentVariableIsSet("BEATGLASS_NATIVE_DJ_TEST")) QSKIP("Set BEATGLASS_NATIVE_DJ_TEST to exercise a real default output");
    const auto endpoints = beatglass::windows::enumerate_render_endpoints();
    QVERIFY(endpoints.error.empty());
    bool haveDefault = false;
    for (const auto& endpoint : endpoints.endpoints) haveDefault = haveDefault || endpoint.is_default;
    if (!haveDefault) QSKIP("No default render endpoint");
    beatglass::SourceEpochAllocator epochs;
    std::atomic<bool> issued{false};
    std::vector<std::uint64_t> offline;
    std::jthread offlineWorker([&](std::stop_token stop) {
      while (!stop.stop_requested()) {
        auto token = epochs.next();
        if (!token) break;
        offline.push_back(*token);
        issued.store(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    });
    QTRY_VERIFY(issued.load());
    {
      beatglass::windows::DjBridge bridge(epochs);
      bridge.start();
      QTRY_COMPARE_WITH_TIMEOUT(bridge.status(), QStringLiteral("Monitoring default output"), 5000);
    }
    QTest::qWait(30);
    offlineWorker.request_stop();
    offlineWorker.join();
    QVERIFY(offline.size() > 2);
    bool gap = false;
    for (std::size_t i = 1; i < offline.size(); ++i) {
      QVERIFY(offline[i] > offline[i - 1]);
      gap = gap || offline[i] > offline[i - 1] + 1;
    }
    QVERIFY2(gap, "Live startup must consume an epoch from the same allocator while the offline worker is issuing");
    QVERIFY(epochs.next().value() > offline.back());
  }
  void exhaustedSharedAllocatorCannotStartCapture() {
    if (!qEnvironmentVariableIsSet("BEATGLASS_NATIVE_DJ_TEST")) QSKIP("Set BEATGLASS_NATIVE_DJ_TEST to exercise a real default output");
    const auto endpoints = beatglass::windows::enumerate_render_endpoints();
    QVERIFY(endpoints.error.empty());
    bool haveDefault = false;
    for (const auto& endpoint : endpoints.endpoints) haveDefault = haveDefault || endpoint.is_default;
    if (!haveDefault) QSKIP("No default render endpoint");
    beatglass::SourceEpochAllocator epochs(std::numeric_limits<std::uint64_t>::max());
    beatglass::windows::DjBridge bridge(epochs);
    QSignalSpy updates(&bridge, &beatglass::windows::DjBridge::presentationUpdate);
    bridge.start();
    QTRY_COMPARE_WITH_TIMEOUT(bridge.status(), QStringLiteral("Capture start failed"), 5000);
    for (const auto& delivery : updates) QVERIFY(!qvariant_cast<beatglass::PresentationUpdate>(delivery.at(0)).snapshot);
    QVERIFY(!epochs.next());
    bridge.stop();
  }
  void fatalFailureRemainsVisibleUntilStop() {
    beatglass::SourceEpochAllocator epochs;
    beatglass::windows::DjBridge bridge(epochs, nullptr, []() -> beatglass::windows::EndpointResult { throw std::runtime_error("failure"); });
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
    beatglass::SourceEpochAllocator epochs;
    beatglass::windows::DjBridge bridge(epochs);
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
