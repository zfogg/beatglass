#include "windows_device_position.hpp"
#include <catch2/catch_test_macros.hpp>
#include <limits>

TEST_CASE("Device frame position gaps and resets require reacquisition") {
  beatglass::windows::DevicePositionContinuity position;
  REQUIRE_FALSE(position.observe(100, 32, true));
  REQUIRE_FALSE(position.observe(132, 32, true));
  REQUIRE(position.observe(200, 32, true));
  REQUIRE_FALSE(position.observe(232, 8, true));
  REQUIRE(position.observe(1, 10, true));
  REQUIRE_FALSE(position.observe(11, 10, true));
}

TEST_CASE("Invalid and overflowing device positions reset the baseline") {
  beatglass::windows::DevicePositionContinuity position;
  REQUIRE_FALSE(position.observe(12, 8, true));
  REQUIRE(position.observe(20, 8, false));
  REQUIRE_FALSE(position.observe(200, 8, true));
  REQUIRE(position.observe(std::numeric_limits<std::uint64_t>::max() - 1, 2, true));
  REQUIRE_FALSE(position.observe(400, 8, true));
  REQUIRE_FALSE(position.observe(408, 0, true));
  REQUIRE_FALSE(position.observe(408, 1, true));
}

TEST_CASE("Capture buffer frame and timestamp arithmetic fails closed") {
  using beatglass::windows::captureBufferArithmeticValid;
  constexpr auto max = std::numeric_limits<std::uint64_t>::max();
  CHECK(captureBufferArithmeticValid(max - 2, 2, max, true, 48000, 2));
  CHECK_FALSE(captureBufferArithmeticValid(max - 2, 3, 0, true, 48000, 2));
  // Four frames in two chunks: the second chunk begins 2 / 48000 seconds later.
  CHECK(captureBufferArithmeticValid(0, 4, max - 416, true, 48000, 2));
  CHECK_FALSE(captureBufferArithmeticValid(0, 4, max - 415, true, 48000, 2));
  CHECK(captureBufferArithmeticValid(0, 4, max, false, 48000, 2));
  CHECK_FALSE(captureBufferArithmeticValid(0, 4, 0, true, 0, 2));
  CHECK_FALSE(captureBufferArithmeticValid(0, 4, 0, true, 48000, 0));
}

TEST_CASE("Empty invalid or discontinuous buffers defer the gap to audio") {
  beatglass::windows::DevicePositionContinuity position;
  CHECK_FALSE(position.observe(10, 10, true));
  CHECK(position.observe(20, 0, false));
  CHECK(position.observe(30, 8, true));
  CHECK_FALSE(position.observe(38, 8, true));
  CHECK(position.observe(46, 0, true, true));
  CHECK(position.observe(46, 8, true));
  CHECK_FALSE(position.observe(54, 8, true));
}
