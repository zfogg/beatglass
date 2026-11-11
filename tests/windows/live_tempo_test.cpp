#include "windows_endpoints.hpp"
#include "windows_live_tempo.hpp"
#include "windows_default_tempo.hpp"
#include <catch2/catch_test_macros.hpp>
#include <windows.h>
#include <mmsystem.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>
#include <deque>
#include <vector>

namespace {
class TempoFakeLoopback final : public beatglass::windows::Loopback {
public:
  State current{State::stopped}; std::uint64_t epoch{}, dropped{};
  std::deque<Event> events; std::deque<beatglass::windows::LoopbackPacket> packets;
  bool start(const std::string&, std::uint64_t e) override { epoch=e; current=State::running; events.push_back({State::running,1,e,0}); return true; }
  void stop() override { current=State::stopped; packets.clear(); events.clear(); }
  State state() const override { return current; }
  bool pop_event(Event& event) override { if(events.empty()) return false; event=events.front(); events.pop_front(); return true; }
  bool pop(beatglass::windows::LoopbackPacket& packet) override { if(packets.empty()) return false; packet=packets.front(); packets.pop_front(); return true; }
  std::uint64_t dropped_frames() const override { return dropped; }
  void add(std::uint64_t frame, std::uint64_t qpc=100000000) {
    beatglass::windows::LoopbackPacket packet; packet.epoch=epoch; packet.first_frame=frame;
    packet.sample_rate=44100; packet.channels=1; packet.sample_count=512; packet.timestamp_valid=true; packet.qpc_100ns=qpc; packet.loss_stamp=lossToken()->stamp();
    for(unsigned i=0;i<packet.sample_count;++i) packet.samples[i]=0.2f*std::sin(float(frame+i)*0.037f);
    packets.push_back(packet);
  }
};
}
TEST_CASE("Live tempo rejects stale future and untimed capture") {
  beatglass::SourceEpochAllocator epochs; auto backend=std::make_unique<TempoFakeLoopback>(); auto* fake=backend.get();
  std::uint64_t now=110000000;
  beatglass::windows::LiveTempo tempo(epochs,std::move(backend),[&] { return std::optional<std::uint64_t>{now}; });
  REQUIRE(tempo.start("fake")); fake->add(0); REQUIRE(tempo.pump()); REQUIRE(tempo.observedThrough());
  const auto generation=tempo.invalidationGeneration(); fake->add(512,now+1);
  REQUIRE(tempo.pump()); CHECK_FALSE(tempo.observedThrough()); CHECK(tempo.invalidationGeneration()>generation);
  fake->add(1024,now-10000000); REQUIRE(tempo.pump()); REQUIRE(tempo.observedThrough());
  fake->add(1536,now-10000001); REQUIRE(tempo.pump()); CHECK_FALSE(tempo.observedThrough());
  fake->add(2048,now); fake->packets.back().timestamp_valid=false;
  REQUIRE(tempo.pump()); CHECK_FALSE(tempo.observedThrough());
}
TEST_CASE("Live tempo invalidates loss or expired capture after DSP") {
  beatglass::SourceEpochAllocator epochs; auto backend=std::make_unique<TempoFakeLoopback>(); auto* fake=backend.get();
  std::uint64_t now=100000000; bool interrupt=false, age=false;
  beatglass::windows::LiveTempo tempo(epochs,std::move(backend),[&] { return std::optional<std::uint64_t>{now}; },[&] {
    if (interrupt) { fake->current=beatglass::windows::Loopback::State::interrupted; fake->events.push_back({fake->current,2,fake->epoch,0}); }
    if (age) now+=10000001;
  });
  REQUIRE(tempo.start("fake")); fake->add(0); REQUIRE(tempo.pump()); REQUIRE(tempo.observedThrough());
  const auto generation=tempo.invalidationGeneration(); fake->add(512);
  SECTION("interruption") { interrupt=true; CHECK_FALSE(tempo.pump()); }
  SECTION("age") { age=true; CHECK(tempo.pump()); }
  CHECK_FALSE(tempo.observedThrough()); CHECK_FALSE(tempo.bpm()); CHECK(tempo.invalidationGeneration()>generation);
}
TEST_CASE("Live tempo fails closed on capture clock error") {
  beatglass::SourceEpochAllocator epochs; auto backend=std::make_unique<TempoFakeLoopback>(); auto* fake=backend.get();
  beatglass::windows::LiveTempo tempo(epochs,std::move(backend),[] { return std::optional<std::uint64_t>{}; });
  REQUIRE(tempo.start("fake")); fake->add(0); CHECK_FALSE(tempo.pump()); CHECK_FALSE(tempo.observedThrough());
}

TEST_CASE("Live tempo starts unknown and invalidates on failure and stop") {
  beatglass::SourceEpochAllocator epochs;
  beatglass::windows::LiveTempo tempo(epochs);
  REQUIRE_FALSE(tempo.bpm());
  REQUIRE(tempo.start("not-a-real-endpoint"));
  for (int i = 0; i < 200 && tempo.state() == beatglass::SourceState::starting; ++i) {
    tempo.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REQUIRE_FALSE(tempo.pump());
  REQUIRE_FALSE(tempo.bpm());
  tempo.stop();
  REQUIRE_FALSE(tempo.bpm());
}

TEST_CASE("Generated playback reaches live aubio tempo through WASAPI", "[.][native-tempo]") {
  const auto list = beatglass::windows::enumerate_render_endpoints();
  REQUIRE(list.error.empty());
  const auto endpoint = std::find_if(list.endpoints.begin(), list.endpoints.end(), [](const auto& item) { return item.is_default; });
  REQUIRE(endpoint != list.endpoints.end());
  INFO("Render endpoint: " << endpoint->name);
  beatglass::SourceEpochAllocator epochs;
  beatglass::windows::LiveTempo tempo(epochs);
  beatglass::SourceEpochAllocator follower_epochs;
  beatglass::windows::DefaultTempoFollower follower(follower_epochs, [&] { return list; });
  REQUIRE(follower.start());
  beatglass::PresentationGate gate;
  REQUIRE(follower.tick(std::chrono::steady_clock::now()));
  auto clear = follower.takeUpdate();
  REQUIRE(clear);
  REQUIRE(gate.apply(*clear));
  REQUIRE(tempo.start(endpoint->id));
  for (int i = 0; i < 300 && tempo.state() == beatglass::SourceState::starting; ++i) {
    const auto pumped = tempo.pump();
    CAPTURE(tempo.error_code());
    REQUIRE(pumped);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REQUIRE(tempo.state() == beatglass::SourceState::running);
  WAVEFORMATEX format{};
  format.wFormatTag = WAVE_FORMAT_PCM; format.nChannels = 1; format.nSamplesPerSec = 44100;
  format.wBitsPerSample = 16; format.nBlockAlign = 2; format.nAvgBytesPerSec = 88200;
  std::vector<short> pcm(44100 * 20);
  for (std::size_t i = 0; i < pcm.size(); ++i) {
    const auto phase = i % 22050;
    if (phase < 1000) pcm[i] = static_cast<short>(10000.0 * std::exp(-static_cast<double>(phase) / 100.0) * std::sin(phase * 0.4));
  }
  HWAVEOUT output{};
  REQUIRE(waveOutOpen(&output, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL) == MMSYSERR_NOERROR);
  struct Output { HWAVEOUT handle; WAVEHDR header{}; bool prepared{}; ~Output() { waveOutReset(handle); if (prepared) waveOutUnprepareHeader(handle, &header, sizeof(header)); waveOutClose(handle); } } playback{output};
  playback.header.lpData = reinterpret_cast<LPSTR>(pcm.data());
  playback.header.dwBufferLength = static_cast<DWORD>(pcm.size() * sizeof(short));
  REQUIRE(waveOutPrepareHeader(output, &playback.header, sizeof(playback.header)) == MMSYSERR_NOERROR);
  playback.prepared = true;
  REQUIRE(waveOutWrite(output, &playback.header, sizeof(playback.header)) == MMSYSERR_NOERROR);
  std::optional<beatglass::PresentationUpdate> prior_snapshot;
  for (int i = 0; i < 2000 && !tempo.bpm(); ++i) {
    const auto pumped = tempo.pump();
    CAPTURE(tempo.error_code(), tempo.state());
    REQUIRE(pumped);
    REQUIRE(follower.tick(std::chrono::steady_clock::now()));
    if (auto update = follower.takeUpdate()) { if (update->snapshot) prior_snapshot = *update; gate.apply(*update); }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REQUIRE(tempo.bpm());
  INFO("Detected BPM: " << *tempo.bpm());
  const auto bpm = *tempo.bpm();
  REQUIRE((std::abs(bpm - 60.0f) < 6.0f || std::abs(bpm - 120.0f) < 12.0f || std::abs(bpm - 240.0f) < 24.0f));
  for (int i = 0; i < 400 && !gate.current(); ++i) {
    REQUIRE(follower.tick(std::chrono::steady_clock::now()));
    if (auto update = follower.takeUpdate()) { if (update->snapshot) prior_snapshot = *update; gate.apply(*update); }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REQUIRE(gate.current());
  REQUIRE(gate.current()->bpm);
  REQUIRE(waveOutReset(output) == MMSYSERR_NOERROR);
  for (int i = 0; i < 300 && tempo.bpm(); ++i) {
    REQUIRE(tempo.pump());
    REQUIRE(follower.tick(std::chrono::steady_clock::now()));
    if (auto update = follower.takeUpdate()) gate.apply(*update);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REQUIRE_FALSE(tempo.bpm());
  for (int i = 0; i < 300 && gate.current(); ++i) {
    REQUIRE(follower.tick(std::chrono::steady_clock::now()));
    if (auto update = follower.takeUpdate()) gate.apply(*update);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REQUIRE_FALSE(gate.current());
  REQUIRE(prior_snapshot);
  REQUIRE_FALSE(gate.apply(*prior_snapshot));
  REQUIRE_FALSE(gate.current());
  tempo.stop();
  REQUIRE_FALSE(tempo.bpm());
}

TEST_CASE("Live tempo rejects queued stamps when capture loss races DSP") {
  beatglass::SourceEpochAllocator epochs; auto backend=std::make_unique<TempoFakeLoopback>(); auto* fake=backend.get();
  auto token=std::const_pointer_cast<beatglass::windows::LoopbackLossToken>(fake->lossToken()); bool lose=false;
  beatglass::windows::LiveTempo tempo(epochs,std::move(backend),[] { return std::optional<std::uint64_t>{100000000}; },[&] { if (lose) token->invalidate(); });
  REQUIRE(tempo.start("fake")); fake->add(0); REQUIRE(tempo.pump()); REQUIRE(tempo.lossStamp());
  const auto queued=*tempo.lossStamp(); REQUIRE(token->matches(queued));
  fake->add(512); lose=true; REQUIRE(tempo.pump()); CHECK_FALSE(tempo.observedThrough());
  CHECK_FALSE(tempo.lossStamp()); CHECK_FALSE(token->matches(queued));
  lose=false; fake->add(1024); REQUIRE(tempo.pump()); REQUIRE(tempo.lossStamp());
  CHECK(token->matches(*tempo.lossStamp())); CHECK_FALSE(token->matches(queued));
}
