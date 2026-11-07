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
#include <vector>

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
    REQUIRE(tempo.pump());
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
    REQUIRE(tempo.pump());
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
