#include "windows_dj.hpp"
#include <QtTest>
#include <QSignalSpy>
#include "windows_endpoints.hpp"
#include "windows_live_tempo.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <windows.h>
#include <mmsystem.h>
#include <algorithm>
#include <cmath>
#include <vector>
namespace {
struct SelectedFakeState {
  std::atomic<bool> available{true};
  std::atomic<bool> pending{false};
  std::atomic<unsigned> attempts{0};
  std::atomic<std::uint64_t> lastEpoch{0};
  std::atomic<bool> blockNextStop{false}, stopEntered{false};
  std::mutex mutex;
  std::condition_variable wake;
  bool releaseStop{};
  std::vector<std::string> endpoints;
};
struct DiscoveryGate {
  std::atomic<bool> entered{false};
  std::mutex mutex;
  std::condition_variable wake;
  bool released{};
  beatglass::windows::EndpointResult discover() {
    std::unique_lock lock(mutex);
    entered = true;
    wake.wait(lock, [&] { return released; });
    return {{{"not-a-real-default", "default", true}}, {}};
  }
  void release() { { std::lock_guard lock(mutex); released = true; } wake.notify_all(); }
};
class SelectedFake final : public beatglass::windows::LiveTempo {
public:
  SelectedFake(beatglass::SourceEpochAllocator& epochs, SelectedFakeState& shared)
      : LiveTempo(epochs), epochs_(epochs), shared_(shared) {}
  bool start(const std::string& endpoint) override {
    ++shared_.attempts;
    auto epoch = epochs_.next();
    if (!epoch) return false;
    shared_.lastEpoch = *epoch;
    { std::lock_guard lock(shared_.mutex); shared_.endpoints.push_back(endpoint); }
    running_ = true;
    return true;
  }
  bool pump() override { return running_ && shared_.available.load(); }
  std::optional<float> bpm() const override { return running_ && !shared_.pending.load() ? std::optional<float>(120.0f) : std::nullopt; }
  beatglass::SourceState state() const override { return !running_ ? beatglass::SourceState::stopped : shared_.pending.load() ? beatglass::SourceState::starting : beatglass::SourceState::running; }
  void stop() override {
    if (shared_.blockNextStop.exchange(false)) {
      std::unique_lock lock(shared_.mutex);
      shared_.stopEntered = true;
      shared_.wake.wait(lock, [&] { return shared_.releaseStop; });
    }
    running_ = false;
  }
private:
  beatglass::SourceEpochAllocator& epochs_;
  SelectedFakeState& shared_;
  bool running_{};
};
}
class WindowsDjTest : public QObject {
 Q_OBJECT
private slots:
 void stopOrSwitchWhileAcceptedStartIsPending() {
   for (bool changeEndpoint : {false, true}) {
     beatglass::SourceEpochAllocator epochs;
     SelectedFakeState shared;
     shared.pending = true;
     auto capture = std::make_unique<SelectedFake>(epochs, shared);
     WindowsDj model(epochs, std::move(capture));
     model.start(QStringLiteral("old"));
     QTRY_COMPARE_WITH_TIMEOUT(shared.attempts.load(), 1U, 5000);
     QTRY_COMPARE_WITH_TIMEOUT(model.status(), QStringLiteral("Starting"), 5000);
     if (changeEndpoint) model.start(QStringLiteral("new"));
     else model.stop();
     shared.pending = false;
     if (changeEndpoint) {
       QTRY_COMPARE_WITH_TIMEOUT(shared.attempts.load(), 2U, 5000);
       QTRY_COMPARE_WITH_TIMEOUT(model.tempo(), QStringLiteral("120.0"), 5000);
       std::lock_guard lock(shared.mutex);
       QCOMPARE(shared.endpoints.size(), std::size_t(2));
       QCOMPARE(shared.endpoints[1], std::string("new"));
     } else {
       QTest::qWait(300);
       QCOMPARE(shared.attempts.load(), 1U);
       QCOMPARE(model.status(), QStringLiteral("Stopped"));
       QCOMPARE(model.tempo(), QStringLiteral("\u2014"));
     }
   }
 }
 void destructionAtRetryDeadlineCannotReopenEndpoint() {
   beatglass::SourceEpochAllocator epochs;
   SelectedFakeState shared;
   auto capture = std::make_unique<SelectedFake>(epochs, shared);
   auto model = std::make_unique<WindowsDj>(epochs, std::move(capture));
   struct Release {
     SelectedFakeState& state;
     ~Release() { { std::lock_guard lock(state.mutex); state.releaseStop = true; } state.wake.notify_all(); }
   } release{shared};
   model->start(QStringLiteral("old"));
   QTRY_COMPARE_WITH_TIMEOUT(model->tempo(), QStringLiteral("120.0"), 5000);
   shared.available = false;
   QTRY_COMPARE_WITH_TIMEOUT(model->status(), QStringLiteral("Output interrupted; retrying"), 5000);
   shared.blockNextStop = true;
   QTRY_VERIFY_WITH_TIMEOUT(shared.stopEntered.load(), 5000);
   std::thread unblock([&] {
     std::this_thread::sleep_for(std::chrono::milliseconds(100));
     { std::lock_guard lock(shared.mutex); shared.releaseStop = true; }
     shared.wake.notify_all();
   });
   model.reset();
   unblock.join();
   QCOMPARE(shared.attempts.load(), 1U);
 }
 void stopOrSwitchAtRetryDeadlineCannotReopenOldEndpoint() {
   for (bool changeEndpoint : {false, true}) {
     beatglass::SourceEpochAllocator epochs;
     SelectedFakeState shared;
     auto capture = std::make_unique<SelectedFake>(epochs, shared);
     WindowsDj model(epochs, std::move(capture));
     struct Release {
       SelectedFakeState& state;
       ~Release() { { std::lock_guard lock(state.mutex); state.releaseStop = true; } state.wake.notify_all(); }
     } release{shared};
     model.start(QStringLiteral("old"));
     QTRY_COMPARE_WITH_TIMEOUT(model.tempo(), QStringLiteral("120.0"), 5000);
     shared.available = false;
     QTRY_COMPARE_WITH_TIMEOUT(model.status(), QStringLiteral("Output interrupted; retrying"), 5000);
     shared.blockNextStop = true;
     QTRY_VERIFY_WITH_TIMEOUT(shared.stopEntered.load(), 5000);
     if (changeEndpoint) { shared.available = true; model.start(QStringLiteral("new")); }
     else model.stop();
     { std::lock_guard lock(shared.mutex); shared.releaseStop = true; }
     shared.wake.notify_all();
     if (changeEndpoint) {
       QTRY_COMPARE_WITH_TIMEOUT(shared.attempts.load(), 2U, 5000);
       std::lock_guard lock(shared.mutex);
       QCOMPARE(shared.endpoints.size(), std::size_t(2));
       QCOMPARE(shared.endpoints[0], std::string("old"));
       QCOMPARE(shared.endpoints[1], std::string("new"));
     } else {
       QTest::qWait(300);
       QCOMPARE(shared.attempts.load(), 1U);
       QCOMPARE(model.status(), QStringLiteral("Stopped"));
     }
   }
 }
 void defaultDiscoveryCannotStartAfterStopOrSwitch() {
   for (bool changeEndpoint : {false, true}) {
     beatglass::SourceEpochAllocator epochs;
     SelectedFakeState selected;
     DiscoveryGate gate;
     auto capture = std::make_unique<SelectedFake>(epochs, selected);
     WindowsDj model(epochs, std::move(capture), [&] { return gate.discover(); });
     struct Release { DiscoveryGate& gate; ~Release() { gate.release(); } } release{gate};
     model.start(QStringLiteral("__beatglass_default__"));
     QTRY_VERIFY_WITH_TIMEOUT(gate.entered.load(), 5000);
     if (changeEndpoint) model.start(QStringLiteral("new"));
     else model.stop();
     gate.release();
     if (changeEndpoint) {
       QTRY_COMPARE_WITH_TIMEOUT(selected.attempts.load(), 1U, 5000);
       QCOMPARE(selected.lastEpoch.load(), std::uint64_t(1));
       std::lock_guard lock(selected.mutex);
       QCOMPARE(selected.endpoints.front(), std::string("new"));
     } else {
       QTest::qWait(300);
       QCOMPARE(model.status(), QStringLiteral("Stopped"));
       QCOMPARE(epochs.next().value(), std::uint64_t(1));
     }
   }
 }
 void defaultDiscoveryCannotStartAfterDestruction() {
   beatglass::SourceEpochAllocator epochs;
   SelectedFakeState selected;
   DiscoveryGate gate;
   auto capture = std::make_unique<SelectedFake>(epochs, selected);
   auto model = std::make_unique<WindowsDj>(epochs, std::move(capture), [&] { return gate.discover(); });
   struct Release { DiscoveryGate& gate; ~Release() { gate.release(); } } release{gate};
   model->start(QStringLiteral("__beatglass_default__"));
   QTRY_VERIFY_WITH_TIMEOUT(gate.entered.load(), 5000);
   std::thread unblock([&] { std::this_thread::sleep_for(std::chrono::milliseconds(100)); gate.release(); });
   model.reset();
   unblock.join();
   QCOMPARE(epochs.next().value(), std::uint64_t(1));
 }
 void stopOrSwitchWhileWorkerIsBlockedDoesNotStartStaleEndpoint() {
   for (bool changeEndpoint : {false, true}) {
     beatglass::SourceEpochAllocator epochs;
     SelectedFakeState shared;
     shared.blockNextStop = true;
     auto capture = std::make_unique<SelectedFake>(epochs, shared);
     WindowsDj model(epochs, std::move(capture));
     struct Release {
       SelectedFakeState& state;
       ~Release() { { std::lock_guard lock(state.mutex); state.releaseStop = true; } state.wake.notify_all(); }
     } release{shared};
     model.start(QStringLiteral("old"));
     QTRY_VERIFY_WITH_TIMEOUT(shared.stopEntered.load(), 5000);
     if (changeEndpoint) model.start(QStringLiteral("new"));
     else model.stop();
     { std::lock_guard lock(shared.mutex); shared.releaseStop = true; }
     shared.wake.notify_all();
     if (changeEndpoint) {
       QTRY_COMPARE_WITH_TIMEOUT(shared.attempts.load(), 1U, 5000);
       std::lock_guard lock(shared.mutex);
       QCOMPARE(shared.endpoints.size(), std::size_t(1));
       QCOMPARE(shared.endpoints.front(), std::string("new"));
     } else {
       QTest::qWait(300);
       QCOMPARE(shared.attempts.load(), 0U);
       QCOMPARE(model.status(), QStringLiteral("Stopped"));
     }
   }
 }
 void destructionWhileWorkerIsBlockedDoesNotStartStaleEndpoint() {
   beatglass::SourceEpochAllocator epochs;
   SelectedFakeState shared;
   shared.blockNextStop = true;
   auto capture = std::make_unique<SelectedFake>(epochs, shared);
   auto model = std::make_unique<WindowsDj>(epochs, std::move(capture));
   struct Release {
     SelectedFakeState& state;
     ~Release() { { std::lock_guard lock(state.mutex); state.releaseStop = true; } state.wake.notify_all(); }
   } release{shared};
   model->start(QStringLiteral("old"));
   QTRY_VERIFY_WITH_TIMEOUT(shared.stopEntered.load(), 5000);
   std::thread unblock([&] {
     std::this_thread::sleep_for(std::chrono::milliseconds(100));
     { std::lock_guard lock(shared.mutex); shared.releaseStop = true; }
     shared.wake.notify_all();
   });
   model.reset();
   unblock.join();
   QCOMPARE(shared.attempts.load(), 0U);
 }
 void selectedOutputReacquiresAfterLossAndStopCancelsRetry() {
   beatglass::SourceEpochAllocator epochs;
   SelectedFakeState shared;
   auto capture = std::make_unique<SelectedFake>(epochs, shared);
   WindowsDj model(epochs, std::move(capture));
   model.start(QStringLiteral("selected"));
   QTRY_COMPARE_WITH_TIMEOUT(model.tempo(), QStringLiteral("120.0"), 5000);
   const auto originalEpoch = shared.lastEpoch.load();
   QVERIFY(originalEpoch != 0);
   shared.available = false;
   QTRY_COMPARE_WITH_TIMEOUT(model.status(), QStringLiteral("Output interrupted; retrying"), 5000);
   QCOMPARE(model.tempo(), QStringLiteral("\u2014"));
   QVERIFY(model.active());
   QTRY_VERIFY_WITH_TIMEOUT(shared.attempts.load() >= 3, 5000);
   QCOMPARE(model.tempo(), QStringLiteral("\u2014"));
   shared.available = true;
   QTRY_COMPARE_WITH_TIMEOUT(model.tempo(), QStringLiteral("120.0"), 5000);
   QVERIFY(shared.lastEpoch.load() > originalEpoch);
   shared.available = false;
   QTRY_COMPARE_WITH_TIMEOUT(model.status(), QStringLiteral("Output interrupted; retrying"), 5000);
   model.stop();
   QCOMPARE(model.status(), QStringLiteral("Stopped"));
   QCOMPARE(model.tempo(), QStringLiteral("\u2014"));
   const auto attempts = shared.attempts.load();
   QTest::qWait(1300);
   QCOMPARE(shared.attempts.load(), attempts);
   QVERIFY(!model.active());
 }
 void sourceListDoesNotResetOnPolling() {
   beatglass::SourceEpochAllocator epochs; WindowsDj model(epochs);
   QTRY_VERIFY_WITH_TIMEOUT(!model.sources().isEmpty(), 5000);
   QSignalSpy spy(&model, &WindowsDj::sourcesChanged);
   model.start(QStringLiteral("not-a-real-endpoint"));
   QTRY_VERIFY_WITH_TIMEOUT(!model.active(), 5000);
   QTest::qWait(250); QCOMPARE(spy.count(), 0);
 }
 void stopInvalidatesPendingFailure() {
   beatglass::SourceEpochAllocator epochs; WindowsDj model(epochs);
   model.start(QStringLiteral("not-a-real-endpoint"));
   QVERIFY(model.active());
   model.stop();
   QCOMPARE(model.status(), QStringLiteral("Stopped"));
   QCOMPARE(model.tempo(), QStringLiteral("\u2014"));
   QTest::qWait(500);
   QCOMPARE(model.status(), QStringLiteral("Stopped"));
   QVERIFY(!model.active());
 }
 void nativeTempoAndStop_data() {
   QTest::addColumn<bool>("followDefault");
   QTest::newRow("selected") << false; QTest::newRow("default") << true;
 }
 void nativeTempoAndStop() {
   QFETCH(bool, followDefault);
   if (!qEnvironmentVariableIsSet("BEATGLASS_NATIVE_DJ_TEST")) QSKIP("Set BEATGLASS_NATIVE_DJ_TEST to exercise real output capture");
   const auto endpoints = beatglass::windows::enumerate_render_endpoints();
   QVERIFY(endpoints.error.empty());
   const auto endpoint = std::find_if(endpoints.endpoints.begin(), endpoints.endpoints.end(), [](const auto& x) { return x.is_default; });
   QVERIFY(endpoint != endpoints.endpoints.end());
   beatglass::SourceEpochAllocator epochs; WindowsDj model(epochs);
   model.start(followDefault ? QStringLiteral("__beatglass_default__") : QString::fromStdString(endpoint->id));
   WAVEFORMATEX format{}; format.wFormatTag = WAVE_FORMAT_PCM; format.nChannels = 1; format.nSamplesPerSec = 44100;
   format.wBitsPerSample = 16; format.nBlockAlign = 2; format.nAvgBytesPerSec = 88200;
   std::vector<short> samples(44100 * 20);
   for (std::size_t i = 0; i < samples.size(); ++i) { auto phase = i % 22050; if (phase < 1000) samples[i] = static_cast<short>(10000.0 * std::exp(-double(phase) / 100.0) * std::sin(phase * 0.4)); }
   HWAVEOUT out{}; QCOMPARE(waveOutOpen(&out, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL), MMSYSERR_NOERROR);
   struct Playback { HWAVEOUT out; WAVEHDR hdr{}; bool prepared{}; ~Playback() { waveOutReset(out); if (prepared) waveOutUnprepareHeader(out, &hdr, sizeof(hdr)); waveOutClose(out); } } playback{out};
   playback.hdr.lpData = reinterpret_cast<LPSTR>(samples.data()); playback.hdr.dwBufferLength = static_cast<DWORD>(samples.size() * sizeof(short));
   QCOMPARE(waveOutPrepareHeader(out, &playback.hdr, sizeof(playback.hdr)), MMSYSERR_NOERROR); playback.prepared = true;
   QCOMPARE(waveOutWrite(out, &playback.hdr, sizeof(playback.hdr)), MMSYSERR_NOERROR);
   QTRY_VERIFY_WITH_TIMEOUT(model.tempo() != QStringLiteral("\u2014"), 20000);
   bool valid = false; const auto bpm = model.tempo().toFloat(&valid); QVERIFY(valid);
   QVERIFY(std::abs(bpm - 60.0f) < 6.0f || std::abs(bpm - 120.0f) < 12.0f || std::abs(bpm - 240.0f) < 24.0f);
   model.stop(); QCOMPARE(model.tempo(), QStringLiteral("\u2014"));
   QTest::qWait(500); QCOMPARE(model.tempo(), QStringLiteral("\u2014")); QVERIFY(!model.active());
 }
 void failedEndpointCannotShowTempo() {
   beatglass::SourceEpochAllocator epochs; WindowsDj model(epochs);
   model.start(QStringLiteral("not-a-real-endpoint"));
   QTRY_VERIFY_WITH_TIMEOUT(!model.active(), 5000);
   QVERIFY(model.status().contains(QStringLiteral("failed"), Qt::CaseInsensitive) || model.status().contains(QStringLiteral("could not"), Qt::CaseInsensitive));
   QCOMPARE(model.tempo(), QStringLiteral("\u2014"));
 }
};
QTEST_GUILESS_MAIN(WindowsDjTest)
#include "windows_dj_test.moc"
