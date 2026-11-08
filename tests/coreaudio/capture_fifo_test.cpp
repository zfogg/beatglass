#include "beatglass/capture/coreaudio_fifo_internal.hpp"
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <limits>
#include <atomic>
#include <thread>
#include <vector>
using namespace beatglass::coreaudio;

TEST_CASE("callback gate closes before draining an in-flight callback", "[coreaudio]") {
  CallbackGate gate;
  std::atomic<bool> entered{false}, release{false};
  std::thread callback([&] {
    if (gate.try_enter() != CallbackEntry::entered) return;
    entered.store(true, std::memory_order_release);
    while (!release.load(std::memory_order_acquire)) std::this_thread::yield();
    gate.leave();
  });
  while (!entered.load(std::memory_order_acquire)) std::this_thread::yield();
  CHECK(gate.try_enter() == CallbackEntry::busy);
  gate.close();
  CHECK_FALSE(gate.drained());
  CHECK(gate.try_enter() == CallbackEntry::closed);
  release.store(true, std::memory_order_release);
  callback.join();
  REQUIRE(gate.drained());
  CHECK(gate.try_enter() == CallbackEntry::closed);
  gate.reopen();
  CHECK(gate.try_enter() == CallbackEntry::entered);
  gate.leave();
  gate.close();
  CHECK(gate.drained());
}

TEST_CASE("Core Audio FIFO retains owned samples and host clock validity", "[coreaudio]") {
  CaptureFifo fifo(17, 48000, 2);
  std::array<float, 4> input{1, 2, 3, 4};
  REQUIRE(fifo.push(input, 123, true, false, 100.0, true));
  input.fill(0);
  CaptureBlock block;
  REQUIRE(fifo.pop(block));
  CHECK(block.epoch == 17);
  CHECK(block.first_frame == 0);
  CHECK(block.host_time_valid);
  CHECK(block.host_time == 123);
  CHECK(block.source_sample_time_valid);
  CHECK(block.source_sample_time == 100.0);
  CHECK(block.sample_rate == 48000);
  CHECK(block.channels == 2);
  CHECK(block.sample_count == 4);
  CHECK(block.samples[2] == 3);
  CHECK_FALSE(block.discontinuity);
  CHECK_FALSE(fifo.pop(block));
}

TEST_CASE("Core Audio FIFO preserves explicit source discontinuity", "[coreaudio]") {
  CaptureFifo fifo(23, 48000, 2);
  std::array<float, 2> input{1, 2};
  REQUIRE(fifo.push(input, 77, true, true));
  CaptureBlock block;
  REQUIRE(fifo.pop(block));
  CHECK(block.discontinuity);
  CHECK(block.epoch == 23);
  CHECK(block.first_frame == 0);
  CHECK(block.host_time == 77);
}

TEST_CASE("Core Audio FIFO reports loss and advances sample position", "[coreaudio]") {
  CaptureFifo fifo(1, 44100, 2);
  std::array<float, 4> input{};
  for (int i = 0; i < 15; ++i) REQUIRE(fifo.push(input, 0, false));
  CHECK_FALSE(fifo.push(input, 0, false));
  CHECK(fifo.dropped_frames() == 2);
  CaptureBlock block;
  REQUIRE(fifo.pop(block));
  REQUIRE(fifo.push(input, 0, false));
  for (int i = 0; i < 14; ++i) REQUIRE(fifo.pop(block));
  REQUIRE(fifo.pop(block));
  CHECK(block.first_frame == 32);
  CHECK(block.discontinuity);
  CHECK_FALSE(block.host_time_valid);
  CHECK_FALSE(block.source_sample_time_valid);
}

TEST_CASE("Core Audio FIFO rejects malformed or oversized blocks", "[coreaudio]") {
  CaptureFifo fifo(2, 48000, 2);
  std::array<float, 3> malformed{};
  CHECK_FALSE(fifo.push(malformed, 0, false));
  std::vector<float> oversized(CaptureBlock::max_samples + 2);
  CHECK_FALSE(fifo.push(oversized, 0, false));
  std::array<float, 2> valid{};
  REQUIRE(fifo.push(valid, 0, false));
  CaptureBlock block;
  REQUIRE(fifo.pop(block));
  CHECK(block.discontinuity);
  CHECK(block.first_frame == 1 + oversized.size() / 2);
}

TEST_CASE("Core Audio sample-time gaps and missing timestamps force reacquisition", "[coreaudio]") {
  NativeTimeline timeline;
  CHECK_FALSE(timeline.observe(100, true, 128));
  CHECK_FALSE(timeline.observe(228, true, 128));
  CHECK(timeline.observe(400, true, 128));
  CHECK_FALSE(timeline.observe(528, true, 128));
  CHECK(timeline.observe(0, false, 128));
  CHECK_FALSE(timeline.observe(0, false, 128));
  CHECK(timeline.observe(1000, true, 128));
  CHECK_FALSE(timeline.observe(1128, true, 128));
  CHECK(timeline.observe(std::numeric_limits<double>::quiet_NaN(), true, 128));
  CHECK(timeline.observe(2000, true, 128));
}

TEST_CASE("native cleanup stops on failure and retries remaining dependencies in order", "[coreaudio]") {
  struct State { std::array<bool, 4> live{true, true, true, true}; std::array<int, 8> calls{}; int used = 0; int fail = -1; };
  const CleanupOps ops{
    [](void* p) noexcept { auto& s = *static_cast<State*>(p); if (!s.live[0]) return true; s.calls[s.used++] = 0; if (s.fail == 0) return false; s.live[0] = false; return true; },
    [](void* p) noexcept { auto& s = *static_cast<State*>(p); if (!s.live[1]) return true; s.calls[s.used++] = 1; if (s.fail == 1) return false; s.live[1] = false; return true; },
    [](void* p) noexcept { auto& s = *static_cast<State*>(p); if (!s.live[2]) return true; s.calls[s.used++] = 2; if (s.fail == 2) return false; s.live[2] = false; return true; },
    [](void* p) noexcept { auto& s = *static_cast<State*>(p); if (!s.live[3]) return true; s.calls[s.used++] = 3; if (s.fail == 3) return false; s.live[3] = false; return true; }
  };
  for (int failure = 0; failure != 4; ++failure) {
    State state; state.fail = failure;
    CHECK_FALSE(cleanup_in_order(&state, ops));
    REQUIRE(state.used == failure + 1);
    for (int i = 0; i != 4; ++i) CHECK(state.live[i] == (i >= failure));
    state.fail = -1;
    REQUIRE(cleanup_in_order(&state, ops));
    for (bool live : state.live) CHECK_FALSE(live);
    REQUIRE(state.used == 5);
    CHECK(state.calls[failure] == failure);
    CHECK(state.calls[failure + 1] == failure);
    for (int i = failure + 1; i != 4; ++i) CHECK(state.calls[i + 1] == i);
  }
}
