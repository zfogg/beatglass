#include "playback_guard.hpp"
#include <QtTest>
#include <QMediaDevices>
#include <QAudioDevice>
class PlaybackGuardTest : public QObject {
 Q_OBJECT
private slots:
 void rejectsUnknownPositions() {
   QVERIFY(PlaybackGuard::invalidChannelMask(0));
   QVERIFY(PlaybackGuard::invalidChannelMask(1));
   QVERIFY(PlaybackGuard::invalidChannelMask(7));
   QVERIFY(!PlaybackGuard::invalidChannelMask(6));
 }
 void reportsActualDefaultDevice() {
#ifdef Q_OS_WIN
   const auto device = QMediaDevices::defaultAudioOutput();
   if (device.isNull()) QSKIP("No playback output on this machine");
   const auto previous = qgetenv("QT_MEDIA_BACKEND");
   qputenv("QT_MEDIA_BACKEND", "ffmpeg");
   PlaybackGuard guard;
   const auto invalid = PlaybackGuard::invalidChannelMask(static_cast<std::uint32_t>(device.channelConfiguration()));
   QCOMPARE(guard.diagnostic(device).isEmpty(), !invalid);
   qputenv("QT_MEDIA_BACKEND", "windows");
   QVERIFY(guard.diagnostic(device).isEmpty());
   if (previous.isNull()) qunsetenv("QT_MEDIA_BACKEND"); else qputenv("QT_MEDIA_BACKEND", previous);
#else
   QSKIP("Windows FFmpeg channel-mask check");
#endif
 }
 void reportsMissingDevice() {
   PlaybackGuard guard;
   QVERIFY(!guard.diagnostic(QAudioDevice{}).isEmpty());
 }
};
QTEST_GUILESS_MAIN(PlaybackGuardTest)
#include "playback_guard_test.moc"
