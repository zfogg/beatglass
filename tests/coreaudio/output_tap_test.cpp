#include "beatglass/capture/coreaudio_tap_internal.hpp"
#include <catch2/catch_test_macros.hpp>
using namespace beatglass::coreaudio;

TEST_CASE("Core Audio tap rejects missing UID or epoch without starting", "[coreaudio]") {
  OutputTap tap;
  CHECK(tap.status().state == TapState::stopped);
  CHECK_FALSE(tap.start("", 1));
  CHECK(tap.status().state == TapState::error);
  CHECK_FALSE(tap.start("test", 0));
  CHECK(tap.status().state == TapState::error);
  CHECK_FALSE(tap.start("org.beatglass.test.missing-output-device", 1));
  CHECK(tap.status().state == TapState::error);
  tap.stop();
  CHECK(tap.status().state == TapState::stopped);
  CaptureBlock block;
  CHECK_FALSE(tap.pop(block));
}
