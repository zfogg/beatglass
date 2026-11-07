#include "windows_endpoints.hpp"
#include "windows_loopback.hpp"
#include "windows_loopback_session.hpp"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>
#include <future>
#include <latch>
#include <algorithm>
#include <cmath>
#include <vector>
#include <deque>
#include <windows.h>
#include <mmsystem.h>

using namespace std::chrono_literals;
namespace {
class FakeLoopback final : public beatglass::windows::Loopback {
public:
  bool start(const std::string&, std::uint64_t epoch) override {
    current = State::running; currentEpoch = epoch; dropped = 0; events.push_back({State::running, 1, epoch, 0}); return true;
  }
  bool start(const std::string& endpoint, std::uint64_t epoch, std::shared_ptr<beatglass::windows::LoopbackStartAuthorization> auth) override {
    return auth && !auth->cancelled() && start(endpoint, epoch);
  }
  void stop() override { current = State::stopped; packets.clear(); events.clear(); ++stops; }
  State state() const override { return current; }
  bool pop_event(Event& event) override {
    if (events.empty()) return false;
    event = events.front(); events.pop_front(); return true;
  }
  bool pop(beatglass::windows::LoopbackPacket& packet) override {
    if (packets.empty()) return false;
    packet = packets.front(); packets.pop_front();
    if (dropDuringPop) { dropped += 64; dropDuringPop = false; }
    return true;
  }
  std::uint64_t dropped_frames() const override { return dropped; }
  State current{State::stopped};
  std::uint64_t currentEpoch{}, dropped{};
  unsigned stops{};
  bool dropDuringPop{};
  std::deque<Event> events;
  std::deque<beatglass::windows::LoopbackPacket> packets;
};
}
TEST_CASE("Queue overflow invalidates and discards pre-gap PCM before fresh-epoch reacquisition") {
  beatglass::SourceEpochAllocator epochs;
  unsigned invalidations = 0;
  auto backend = std::make_unique<FakeLoopback>();
  auto* fake = backend.get();
  beatglass::windows::LoopbackSession session(epochs, [&] { ++invalidations; }, std::move(backend));
  REQUIRE(session.start("fake"));
  session.poll();
  REQUIRE(session.state() == beatglass::SourceState::running);
  beatglass::windows::LoopbackPacket pre{};
  pre.epoch = fake->currentEpoch; pre.first_frame = 10; pre.timestamp_valid = true;
  fake->packets.push_back(pre); fake->packets.push_back(pre);
  fake->dropped = 64;
  beatglass::windows::LoopbackPacket delivered{};
  REQUIRE_FALSE(session.pop(delivered));
  REQUIRE(session.state() == beatglass::SourceState::error);
  REQUIRE(invalidations == 1);
  REQUIRE(fake->packets.empty());
  REQUIRE_FALSE(session.pop(delivered));
  const auto previousEpoch = pre.epoch;
  session.stop();
  REQUIRE(session.start("fake"));
  session.poll();
  REQUIRE(fake->currentEpoch > previousEpoch);
  beatglass::windows::LoopbackPacket fresh{};
  fresh.epoch = fake->currentEpoch; fresh.first_frame = 0; fresh.timestamp_valid = true;
  fake->packets.push_back(fresh);
  REQUIRE(session.pop(delivered));
  REQUIRE(delivered.epoch == fresh.epoch);
  fake->packets.push_back(fresh);
  fake->dropDuringPop = true;
  const auto before = invalidations;
  REQUIRE_FALSE(session.pop(delivered));
  REQUIRE(invalidations == before + 1);
  REQUIRE(session.state() == beatglass::SourceState::error);
  REQUIRE(fake->packets.empty());
}
TEST_CASE("Session invalidates before shutdown and preserves process epochs") {
  beatglass::SourceEpochAllocator epochs;
  beatglass::windows::LoopbackSession no_invalidation(epochs, {});
  REQUIRE_FALSE(no_invalidation.start("any endpoint"));
  REQUIRE(no_invalidation.state() == beatglass::SourceState::stopped);
  unsigned invalidations = 0;
  {
    beatglass::windows::LoopbackSession session(epochs, [&] { ++invalidations; });
    REQUIRE_FALSE(session.start(""));
    REQUIRE(session.state() == beatglass::SourceState::error);
    REQUIRE(invalidations == 1);
    session.stop();
    REQUIRE(session.state() == beatglass::SourceState::stopped);
  }
  const auto next = epochs.next();
  REQUIRE(next);
  REQUIRE(*next == 2);
}

TEST_CASE("Session applies running before packets and invalidates on stop") {
  const auto list = beatglass::windows::enumerate_render_endpoints();
  REQUIRE(list.error.empty());
  if (list.endpoints.empty()) SKIP("No active render endpoint on this machine");
  beatglass::SourceEpochAllocator epochs;
  unsigned invalidations = 0;
  beatglass::windows::LoopbackSession session(epochs, [&] { ++invalidations; });
  REQUIRE(session.start(list.endpoints.front().id));
  beatglass::windows::LoopbackPacket packet;
  for (int i = 0; i < 300 && session.state() == beatglass::SourceState::starting; ++i) {
    session.poll();
    std::this_thread::sleep_for(10ms);
  }
  REQUIRE(session.state() == beatglass::SourceState::running);
  REQUIRE(invalidations == 0);
  session.stop();
  REQUIRE(invalidations == 1);
  REQUIRE_FALSE(session.pop(packet));
  REQUIRE(session.start(list.endpoints.front().id));
  session.stop();
  REQUIRE(invalidations == 2);
}

TEST_CASE("Asynchronous endpoint failure invalidates and needs stop before restart") {
  beatglass::SourceEpochAllocator epochs;
  unsigned invalidations = 0;
  beatglass::windows::LoopbackSession session(epochs, [&] { ++invalidations; });
  REQUIRE(session.start("not-a-real-endpoint"));
  for (int i = 0; i < 200 && session.state() == beatglass::SourceState::starting; ++i) {
    session.poll();
    std::this_thread::sleep_for(10ms);
  }
  REQUIRE(session.state() == beatglass::SourceState::error);
  REQUIRE(invalidations == 1);
  REQUIRE_FALSE(session.start("not-a-real-endpoint"));
  session.stop();
  REQUIRE(session.start("not-a-real-endpoint"));
  session.stop();
}
TEST_CASE("Invalid endpoint interrupts and can be stopped and retried") {
  beatglass::windows::Loopback capture;
  REQUIRE_FALSE(capture.start("", 1));
  REQUIRE(capture.start("not-a-real-endpoint", 1));
  for (int i = 0; i < 200 && capture.state() == beatglass::windows::Loopback::State::starting; ++i) std::this_thread::sleep_for(10ms);
  REQUIRE(capture.state() == beatglass::windows::Loopback::State::interrupted);
  REQUIRE(capture.error_code() != 0);
  beatglass::windows::Loopback::Event event;
  REQUIRE(capture.pop_event(event));
  REQUIRE(event.state == beatglass::windows::Loopback::State::interrupted);
  REQUIRE(event.sequence == 1);
  REQUIRE(event.epoch == 1);
  REQUIRE(event.error_code != 0);
  REQUIRE_FALSE(capture.pop_event(event));
  beatglass::windows::LoopbackPacket packet;
  REQUIRE_FALSE(capture.pop(packet));
  capture.stop();
  REQUIRE(capture.state() == beatglass::windows::Loopback::State::stopped);
  REQUIRE(capture.start("not-a-real-endpoint", 2));
  capture.stop();
}

TEST_CASE("Selected active render endpoint starts and stops loopback when available") {
  const auto list = beatglass::windows::enumerate_render_endpoints();
  REQUIRE(list.error.empty());
  if (list.endpoints.empty()) SKIP("No active render endpoint on this machine");
  beatglass::windows::Loopback capture;
  REQUIRE(capture.start(list.endpoints.front().id, 42));
  for (int i = 0; i < 300 && capture.state() == beatglass::windows::Loopback::State::starting; ++i) std::this_thread::sleep_for(10ms);
  INFO("WASAPI HRESULT: " << capture.error_code());
  REQUIRE(capture.state() == beatglass::windows::Loopback::State::running);
  beatglass::windows::Loopback::Event event;
  REQUIRE(capture.pop_event(event));
  REQUIRE(event.state == beatglass::windows::Loopback::State::running);
  REQUIRE(event.sequence == 1);
  REQUIRE(event.epoch == 42);
  REQUIRE_FALSE(capture.start(list.endpoints.front().id, 43));
  capture.stop();
  REQUIRE(capture.state() == beatglass::windows::Loopback::State::stopped);
  beatglass::windows::LoopbackPacket packet;
  REQUIRE_FALSE(capture.pop(packet));
}

TEST_CASE("Repeated capture startup and shutdown discard prior attempt events") {
  const auto list = beatglass::windows::enumerate_render_endpoints();
  REQUIRE(list.error.empty());
  if (list.endpoints.empty()) SKIP("No active render endpoint on this machine");
  beatglass::windows::Loopback capture;
  for (std::uint64_t epoch = 1; epoch <= 30; ++epoch) {
    REQUIRE(capture.start(list.endpoints.front().id, epoch));
    if (epoch % 2 == 0) {
      for (int i = 0; i < 200 && capture.state() == beatglass::windows::Loopback::State::starting; ++i) std::this_thread::sleep_for(5ms);
      REQUIRE(capture.state() == beatglass::windows::Loopback::State::running);
      beatglass::windows::Loopback::Event event;
      REQUIRE(capture.pop_event(event));
      REQUIRE(event.sequence == 1);
      REQUIRE(event.epoch == epoch);
    }
    capture.stop();
    beatglass::windows::Loopback::Event event;
    REQUIRE_FALSE(capture.pop_event(event));
    REQUIRE(capture.state() == beatglass::windows::Loopback::State::stopped);
  }
}

TEST_CASE("Switch between two active render endpoints with fresh lifecycle attempts", "[.][native-switch]") {
  const auto list = beatglass::windows::enumerate_render_endpoints();
  REQUIRE(list.error.empty());
  if (list.endpoints.size() < 2) SKIP("At least two active render endpoints are required");
  beatglass::SourceEpochAllocator epochs;
  unsigned invalidations = 0;
  beatglass::windows::LoopbackSession session(epochs, [&] { ++invalidations; });
  for (unsigned index = 0; index < 2; ++index) {
    INFO("Endpoint: " << list.endpoints[index].name);
    REQUIRE(session.start(list.endpoints[index].id));
    for (int i = 0; i < 300 && session.state() == beatglass::SourceState::starting; ++i) {
      session.poll();
      std::this_thread::sleep_for(10ms);
    }
    REQUIRE(session.state() == beatglass::SourceState::running);
    session.stop();
    REQUIRE(invalidations == index + 1);
  }
  REQUIRE(epochs.next() == 3);
}

TEST_CASE("Generated default endpoint playback is captured and overflow marks a gap", "[.][native-audio]") {
  const auto list = beatglass::windows::enumerate_render_endpoints();
  REQUIRE(list.error.empty());
  const auto endpoint = std::find_if(list.endpoints.begin(), list.endpoints.end(), [](const auto& item) { return item.is_default; });
  REQUIRE(endpoint != list.endpoints.end());
  INFO("Multimedia default render endpoint: " << endpoint->name);
  beatglass::windows::Loopback capture;
  REQUIRE(capture.start(endpoint->id, 101));
  for (int i = 0; i < 300 && capture.state() == beatglass::windows::Loopback::State::starting; ++i) std::this_thread::sleep_for(10ms);
  REQUIRE(capture.state() == beatglass::windows::Loopback::State::running);

  WAVEFORMATEX format{};
  format.wFormatTag = WAVE_FORMAT_PCM; format.nChannels = 1; format.nSamplesPerSec = 48000;
  format.wBitsPerSample = 16; format.nBlockAlign = 2; format.nAvgBytesPerSec = 96000;
  std::vector<short> pcm(48000 * 3);
  for (std::size_t i = 0; i < pcm.size(); ++i) pcm[i] = static_cast<short>(1600.0 * std::sin(2.0 * 3.141592653589793 * 440.0 * i / 48000.0));
  HWAVEOUT output{};
  REQUIRE(waveOutOpen(&output, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL) == MMSYSERR_NOERROR);
  struct Output { HWAVEOUT handle; WAVEHDR header{}; bool prepared{}; ~Output() { waveOutReset(handle); if (prepared) waveOutUnprepareHeader(handle, &header, sizeof(header)); waveOutClose(handle); } } playback{output};
  playback.header.lpData = reinterpret_cast<LPSTR>(pcm.data());
  playback.header.dwBufferLength = static_cast<DWORD>(pcm.size() * sizeof(short));
  REQUIRE(waveOutPrepareHeader(output, &playback.header, sizeof(playback.header)) == MMSYSERR_NOERROR);
  playback.prepared = true;
  REQUIRE(waveOutWrite(output, &playback.header, sizeof(playback.header)) == MMSYSERR_NOERROR);
  beatglass::windows::LoopbackPacket packet;
  double peak_tone = 0.0;
  std::uint64_t observed = 0;
  std::uint64_t previous_end = 0;
  for (int i = 0; i < 150 && (observed < 12000 || peak_tone <= 0.001); ++i) {
    while (capture.pop(packet)) {
      REQUIRE(packet.epoch == 101);
      REQUIRE(packet.channels > 0);
      REQUIRE(packet.sample_rate > 0);
      previous_end = packet.first_frame + packet.sample_count / packet.channels;
      double sine = 0.0, cosine = 0.0;
      for (unsigned j = 0; j < packet.sample_count / packet.channels; ++j) {
        const auto angle = 2.0 * 3.141592653589793 * 440.0 * (packet.first_frame + j) / packet.sample_rate;
        const auto sample = packet.samples[j * packet.channels];
        sine += sample * std::sin(angle); cosine += sample * std::cos(angle); ++observed;
      }
      const auto frames = packet.sample_count / packet.channels;
      if (frames >= 200) peak_tone = (std::max)(peak_tone, 2.0 * std::hypot(sine, cosine) / frames);
    }
    std::this_thread::sleep_for(10ms);
  }
  REQUIRE(observed >= 12000);
  INFO("Peak packet 440 Hz amplitude: " << peak_tone);
  REQUIRE(peak_tone > 0.001);
  // Leave the producer running without consuming long enough to fill its queue.
  for (int i = 0; i < 150 && capture.dropped_frames() == 0; ++i) std::this_thread::sleep_for(10ms);
  REQUIRE(capture.dropped_frames() > 0);
  bool discontinuity = false;
  for (int i = 0; i < 100 && !discontinuity; ++i) {
    while (capture.pop(packet)) {
      discontinuity = discontinuity || (packet.discontinuity && packet.first_frame > previous_end);
      previous_end = packet.first_frame + packet.sample_count / packet.channels;
    }
    std::this_thread::sleep_for(10ms);
  }
  REQUIRE(discontinuity);
  REQUIRE(capture.state() == beatglass::windows::Loopback::State::running);
  capture.stop();
  REQUIRE(capture.start(endpoint->id, 102));
  for (int i = 0; i < 300 && capture.state() == beatglass::windows::Loopback::State::starting; ++i) std::this_thread::sleep_for(10ms);
  REQUIRE(capture.state() == beatglass::windows::Loopback::State::running);
  beatglass::windows::Loopback::Event restart;
  REQUIRE(capture.pop_event(restart));
  REQUIRE(restart.state == beatglass::windows::Loopback::State::running);
  REQUIRE(restart.sequence == 1);
  REQUIRE(restart.epoch == 102);
  bool got_fresh_packet = false;
  for (int i = 0; i < 100 && !got_fresh_packet; ++i) {
    if (capture.pop(packet)) {
      REQUIRE(packet.epoch == 102);
      got_fresh_packet = true;
    }
    std::this_thread::sleep_for(10ms);
  }
  REQUIRE(got_fresh_packet);
  capture.stop();
}

TEST_CASE("Cancelled start is rejected before backend claim") {
  beatglass::SourceEpochAllocator epochs;
  auto backend = std::make_unique<FakeLoopback>(); auto* fake = backend.get();
  beatglass::windows::LoopbackSession session(epochs, [] {}, std::move(backend));
  auto cancelled = std::make_shared<beatglass::windows::LoopbackStartAuthorization>();
  cancelled->cancel();
  REQUIRE_FALSE(session.start("old", cancelled));
  CHECK(fake->current == beatglass::windows::Loopback::State::stopped);
  auto replacement = std::make_shared<beatglass::windows::LoopbackStartAuthorization>();
  // A switch cancels the old attempt, then starts only the replacement.
  REQUIRE(session.start("replacement", replacement));
  session.poll(); CHECK(session.state() == beatglass::SourceState::running);
  CHECK(fake->currentEpoch == 1);
  session.stop();
}

TEST_CASE("Cancellation while backend start is blocked stops accepted source") {
  using namespace beatglass::windows;
  class Blocked final : public Loopback {
  public:
    std::promise<void> entered, release;
    std::atomic<unsigned> stops{0};
    bool start(const std::string&, std::uint64_t, std::shared_ptr<LoopbackStartAuthorization>) override {
      entered.set_value(); release.get_future().wait(); return true;
    }
    void stop() override { ++stops; }
  };
  beatglass::SourceEpochAllocator epochs; auto backend=std::make_unique<Blocked>(); auto* fake=backend.get();
  LoopbackSession session(epochs, [] {}, std::move(backend));
  auto authorization=std::make_shared<LoopbackStartAuthorization>();
  auto entered=fake->entered.get_future();
  auto result=std::async(std::launch::async,[&] { return session.start("old",authorization); });
  REQUIRE(entered.wait_for(5s)==std::future_status::ready);
  authorization->cancel(); fake->release.set_value();
  REQUIRE(result.wait_for(5s)==std::future_status::ready);
  CHECK_FALSE(result.get()); CHECK(fake->stops.load()>=1);
  CHECK(session.state()==beatglass::SourceState::stopped);
}

TEST_CASE("Cancellation after accepted start invalidates and discards queued packets") {
  beatglass::SourceEpochAllocator epochs; unsigned clears=0;
  auto backend=std::make_unique<FakeLoopback>(); auto* fake=backend.get();
  beatglass::windows::LoopbackSession session(epochs, [&] { ++clears; }, std::move(backend));
  auto authorization=std::make_shared<beatglass::windows::LoopbackStartAuthorization>();
  REQUIRE(session.start("selected", authorization)); session.poll();
  REQUIRE(session.state()==beatglass::SourceState::running);
  beatglass::windows::LoopbackPacket packet{}; packet.epoch=fake->currentEpoch;
  fake->packets.push_back(packet); authorization->cancel();
  REQUIRE_FALSE(session.pop(packet)); CHECK(clears==1); CHECK(fake->packets.empty());
  CHECK(session.state()==beatglass::SourceState::error);
}

TEST_CASE("Claim and cancellation have ordered concurrent outcomes") {
  using beatglass::windows::LoopbackStartAuthorization;
  SECTION("cancellation completes before a competing claim") {
    LoopbackStartAuthorization authorization; std::latch cancelled{1};
    auto worker = std::async(std::launch::async, [&] { cancelled.wait(); return authorization.claim(); });
    authorization.cancel(); cancelled.count_down();
    CHECK_FALSE(worker.get()); CHECK(authorization.cancelled());
  }
  SECTION("a completed claim can be revoked by another thread") {
    LoopbackStartAuthorization authorization; std::latch claimed{1}, cancelled{1};
    auto worker = std::async(std::launch::async, [&] {
      const bool acquired = authorization.claim(); claimed.count_down(); cancelled.wait();
      return acquired && authorization.cancelled();
    });
    claimed.wait(); authorization.cancel(); cancelled.count_down();
    CHECK(worker.get()); CHECK_FALSE(authorization.claim());
  }
}

TEST_CASE("Loss token rejects queued evidence and recovers with a new packet stamp") {
  beatglass::windows::LoopbackLossToken token;
  const auto queued=token.stamp(); REQUIRE(token.matches(queued));
  token.invalidate(); CHECK_FALSE(token.matches(queued));
  const auto fresh=token.stamp(); CHECK(token.matches(fresh));
  // Delayed queued work cannot be relabeled with a later token.
  token.invalidate(); CHECK_FALSE(token.matches(fresh)); CHECK_FALSE(token.matches(queued));
}
TEST_CASE("Loss token is visible across capture and GUI threads") {
  beatglass::windows::LoopbackLossToken token; const auto pending=token.stamp();
  std::latch begin{1}, invalidated{1};
  auto worker=std::async(std::launch::async,[&] { begin.wait(); token.invalidate(); invalidated.count_down(); });
  begin.count_down(); invalidated.wait(); CHECK_FALSE(token.matches(pending)); worker.get();
}

TEST_CASE("Capture loss events invalidate prior stamps and exhaustion fails closed") {
  using beatglass::windows::LoopbackLossToken;
  using beatglass::windows::LoopbackLossEvents;
  SECTION("queue full") { LoopbackLossToken token; LoopbackLossEvents events(token); auto old=events.packetStamp(false); events.queueFull(); CHECK_FALSE(token.matches(old)); CHECK(token.matches(events.packetStamp(false))); }
  SECTION("flagged discontinuity") { LoopbackLossToken token; LoopbackLossEvents events(token); auto old=events.packetStamp(false); auto after=events.packetStamp(true); CHECK_FALSE(token.matches(old)); CHECK(token.matches(after)); }
  SECTION("empty discontinuity") { LoopbackLossToken token; LoopbackLossEvents events(token); auto old=events.packetStamp(false); events.emptyDiscontinuity(); CHECK_FALSE(token.matches(old)); CHECK(token.matches(events.packetStamp(false))); }
  SECTION("failure") { LoopbackLossToken token; LoopbackLossEvents events(token); auto old=events.packetStamp(false); events.failure(); CHECK_FALSE(token.matches(old)); }
  SECTION("saturation") { const auto max=(std::numeric_limits<std::uint64_t>::max)(); LoopbackLossToken token(max-1); LoopbackLossEvents events(token); REQUIRE(token.matches(max-1)); events.failure(); CHECK_FALSE(token.matches(max-1)); CHECK_FALSE(token.matches(max)); events.queueFull(); CHECK(token.stamp()==max); CHECK_FALSE(token.matches(max)); }
}
