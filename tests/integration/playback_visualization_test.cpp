#include "playback_visualization.hpp"
#include <QAudioBuffer>
#include <QAudioBufferOutput>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QCoreApplication>
#include <QEventLoop>
#include <QMediaPlayer>
#include <QThread>
#include <QUrl>
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <numbers>
#include <limits>
#include <stdexcept>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif
#endif

namespace {
QAudioBuffer tone(unsigned frames, qint64 start = 0, int channels = 1) {
  QAudioFormat format; format.setSampleRate(48000); format.setChannelCount(channels); format.setSampleFormat(QAudioFormat::Float);
  QByteArray data(int(frames * unsigned(channels) * sizeof(float)), 0);
  for (unsigned n = 0; n < frames; ++n) {
    float value = float(0.5 * std::sin(2 * std::numbers::pi * 1000 * n / 48000));
    for (int c = 0; c < channels; ++c) std::memcpy(data.data() + (std::size_t(n) * channels + c) * sizeof(float), &value, sizeof(value));
  }
  return QAudioBuffer(data, format, start);
}
std::optional<PlaybackVisualization::Frame> awaitFrame(PlaybackVisualization& tap) {
  const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(3);
  while (std::chrono::steady_clock::now() < until) {
    if (auto frame = tap.takeLatest()) return frame;
    if (tap.status() != PlaybackVisualization::Status::active) break;
    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  return {};
}
}
TEST_CASE("playback tap processes bounded real PCM and gates retired generations") {
  REQUIRE_THROWS_AS(PlaybackVisualization(0), std::invalid_argument);
  PlaybackVisualization tap(7);
  auto buffer = tone(1024, 0, 2);
  tap.output()->audioBufferReceived(buffer);
  auto frame = awaitFrame(tap); REQUIRE(frame); REQUIRE(frame->value.epoch == 7); REQUIRE(frame->value.first_frame == 0); REQUIRE(frame->value.end_frame == 1024);
  REQUIRE(frame->value.maximum[0] > 0.4f);
  const auto peak = std::max_element(frame->value.magnitude.begin() + 1, frame->value.magnitude.end());
  REQUIRE(std::distance(frame->value.magnitude.begin(), peak) == 21);
  REQUIRE(*peak > 0.35f);
  REQUIRE(tap.revision() == frame->revision); REQUIRE(tap.isCurrent(7, frame->revision)); REQUIRE_FALSE(tap.isCurrent(8, frame->revision));
  tap.retire(); REQUIRE(tap.revision() == std::numeric_limits<unsigned>::max()); REQUIRE_FALSE(tap.isCurrent(7, frame->revision)); REQUIRE_FALSE(tap.takeLatest());
  tap.output()->audioBufferReceived(buffer); REQUIRE_FALSE(tap.takeLatest());
  PlaybackVisualization replacement(8); replacement.output()->audioBufferReceived(buffer);
  auto next = awaitFrame(replacement); REQUIRE(next); REQUIRE(next->value.epoch == 8);
}
TEST_CASE("older same-generation frames cannot replace a newer frame") {
  PlaybackVisualization tap(11);
  tap.output()->audioBufferReceived(tone(1024, 0));
  auto first = awaitFrame(tap); REQUIRE(first);
  tap.output()->audioBufferReceived(tone(1024, 21333));
  auto second = awaitFrame(tap); REQUIRE(second);
  REQUIRE(second->revision > first->revision);
  std::optional<PlaybackVisualization::Frame> displayed;
  auto deliver = [&](const auto& candidate) {
    if (tap.isCurrent(candidate.value.epoch, candidate.revision) &&
        (!displayed || candidate.revision > displayed->revision)) displayed = candidate;
  };
  deliver(*second); deliver(*first); REQUIRE(displayed); REQUIRE(displayed->revision == second->revision);
  REQUIRE(tap.isCurrent(second->value.epoch, second->revision));
  REQUIRE_FALSE(tap.isCurrent(first->value.epoch, first->revision));
  tap.retire();
  if (displayed && !tap.isCurrent(displayed->value.epoch, displayed->revision)) displayed.reset();
  REQUIRE_FALSE(displayed);
}
TEST_CASE("playback tap fails closed on missing timestamps and excessive input") {
  PlaybackVisualization missing(1); missing.output()->audioBufferReceived(tone(1024, -1));
  REQUIRE(missing.status() == PlaybackVisualization::Status::unsupported_audio); REQUIRE_FALSE(missing.isCurrent(1, 1));
  PlaybackVisualization large(2); large.output()->audioBufferReceived(tone(4097));
  REQUIRE(large.status() == PlaybackVisualization::Status::unsupported_audio); REQUIRE_FALSE(large.takeLatest());
}
TEST_CASE("playback tap rejects same-generation timestamp gaps") {
  PlaybackVisualization tap(3); tap.output()->audioBufferReceived(tone(1024, 0));
  REQUIRE(awaitFrame(tap));
  tap.output()->audioBufferReceived(tone(1024, 50000));
  const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(3);
  while (tap.status() == PlaybackVisualization::Status::active && std::chrono::steady_clock::now() < until) std::this_thread::sleep_for(std::chrono::milliseconds(2));
  REQUIRE(tap.status() == PlaybackVisualization::Status::unsupported_audio); REQUIRE_FALSE(tap.takeLatest()); REQUIRE_FALSE(tap.isCurrent(3, 1));
}
TEST_CASE("playback tap converts signed PCM and rejects non-finite samples") {
  QAudioFormat format; format.setSampleRate(48000); format.setChannelCount(1); format.setSampleFormat(QAudioFormat::Int16);
  QByteArray data(1024 * int(sizeof(std::int16_t)), 0);
  for (int i = 0; i < 1024; ++i) { const std::int16_t sample = i % 2 ? 16384 : -16384; std::memcpy(data.data() + i * sizeof(sample), &sample, sizeof(sample)); }
  PlaybackVisualization integerTap(4); integerTap.output()->audioBufferReceived(QAudioBuffer(data, format, 0));
  auto integerFrame = awaitFrame(integerTap); REQUIRE(integerFrame);
  REQUIRE(integerFrame->value.minimum[0] == -0.5f); REQUIRE(integerFrame->value.maximum[0] == 0.5f);

  format.setSampleFormat(QAudioFormat::Float); QByteArray bad(1024 * int(sizeof(float)), 0);
  const auto nan = std::numeric_limits<float>::quiet_NaN(); std::memcpy(bad.data(), &nan, sizeof(nan));
  PlaybackVisualization floatTap(5); floatTap.output()->audioBufferReceived(QAudioBuffer(bad, format, 0));
  const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(3);
  while (floatTap.status() == PlaybackVisualization::Status::active && std::chrono::steady_clock::now() < until) std::this_thread::sleep_for(std::chrono::milliseconds(2));
  REQUIRE(floatTap.status() == PlaybackVisualization::Status::unsupported_audio); REQUIRE_FALSE(floatTap.isCurrent(5, 1));
}
TEST_CASE("playback tap clears prior evidence at end of stream") {
  PlaybackVisualization tap(6); tap.output()->audioBufferReceived(tone(1024)); REQUIRE(awaitFrame(tap));
  tap.output()->audioBufferReceived(QAudioBuffer());
  REQUIRE(tap.status() == PlaybackVisualization::Status::end_of_stream);
  REQUIRE_FALSE(tap.isCurrent(6, 1)); REQUIRE_FALSE(tap.takeLatest());
}
TEST_CASE("retirement suppresses frames while a producer is active") {
  PlaybackVisualization tap(10); auto buffer = tone(1024);
  std::atomic<bool> started{false}, finished{false};
  std::thread producer([&] {
    started.store(true, std::memory_order_release);
    while (!finished.load(std::memory_order_acquire)) tap.output()->audioBufferReceived(buffer);
  });
  while (!started.load(std::memory_order_acquire)) std::this_thread::yield();
  tap.retire(); finished.store(true, std::memory_order_release); producer.join();
  REQUIRE_FALSE(tap.isCurrent(10, 1)); REQUIRE_FALSE(tap.takeLatest());
}
TEST_CASE("Qt player delivers decoded PCM to playback tap", "[native-media]") {
  if (!qEnvironmentVariableIsSet("BEATGLASS_TEST_MEDIA_PCM")) SKIP("Set BEATGLASS_TEST_MEDIA_PCM for the opt-in Qt backend probe");
  PlaybackVisualization tap(9); QAudioOutput audio; audio.setVolume(0); QMediaPlayer player;
  player.setAudioOutput(&audio); player.setAudioBufferOutput(tap.output());
  player.setSource(QUrl::fromLocalFile(QStringLiteral(BEATGLASS_FIXTURES "/tone.aiff")));
  player.play(); auto frame = awaitFrame(tap);
  player.stop(); player.setAudioBufferOutput(nullptr); player.setAudioOutput(nullptr);
  INFO(player.errorString().toStdString()); INFO(int(tap.status()));
  REQUIRE(frame); REQUIRE(frame->value.epoch == 9); REQUIRE(frame->value.sample_rate > 0);
}
int main(int argc, char** argv) {
#ifdef _WIN32
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#ifdef _MSC_VER
  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
  _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
  _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
#endif
  QCoreApplication app(argc, argv);
  return Catch::Session().run(argc, argv);
}
