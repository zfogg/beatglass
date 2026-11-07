#include "beatglass/capture/coreaudio_fifo_internal.hpp"
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <vector>
using namespace beatglass::coreaudio;

TEST_CASE("Core Audio FIFO retains owned samples and host clock validity", "[coreaudio]") {
  CaptureFifo fifo(17, 48000, 2);
  std::array<float, 4> input{1, 2, 3, 4};
  REQUIRE(fifo.push(input, 123, true));
  input.fill(0);
  CaptureBlock block;
  REQUIRE(fifo.pop(block));
  CHECK(block.epoch == 17);
  CHECK(block.first_frame == 0);
  CHECK(block.host_time_valid);
  CHECK(block.host_time == 123);
  CHECK(block.sample_rate == 48000);
  CHECK(block.channels == 2);
  CHECK(block.sample_count == 4);
  CHECK(block.samples[2] == 3);
  CHECK_FALSE(block.discontinuity);
  CHECK_FALSE(fifo.pop(block));
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
