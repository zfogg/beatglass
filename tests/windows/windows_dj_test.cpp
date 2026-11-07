#include "windows_dj.hpp"
#include <QtTest>
#include <QSignalSpy>
#include "windows_endpoints.hpp"
#include <windows.h>
#include <mmsystem.h>
#include <algorithm>
#include <cmath>
#include <vector>
class WindowsDjTest : public QObject {
 Q_OBJECT
private slots:
 void sourceListDoesNotResetOnPolling() {
   WindowsDj model;
   QTRY_VERIFY_WITH_TIMEOUT(!model.sources().isEmpty(), 5000);
   QSignalSpy spy(&model, &WindowsDj::sourcesChanged);
   model.start(QStringLiteral("not-a-real-endpoint"));
   QTRY_VERIFY_WITH_TIMEOUT(!model.active(), 5000);
   QTest::qWait(250); QCOMPARE(spy.count(), 0);
 }
 void stopInvalidatesPendingFailure() {
   WindowsDj model;
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
   WindowsDj model;
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
   WindowsDj model;
   model.start(QStringLiteral("not-a-real-endpoint"));
   QTRY_VERIFY_WITH_TIMEOUT(!model.active(), 5000);
   QVERIFY(model.status().contains(QStringLiteral("failed"), Qt::CaseInsensitive) || model.status().contains(QStringLiteral("could not"), Qt::CaseInsensitive));
   QCOMPARE(model.tempo(), QStringLiteral("\u2014"));
 }
};
QTEST_GUILESS_MAIN(WindowsDjTest)
#include "windows_dj_test.moc"
