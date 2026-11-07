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
