#include "beatglass/capture/coreaudio_tap_internal.hpp"
#include <CoreAudio/CoreAudio.h>
#include <catch2/catch_test_macros.hpp>
using namespace beatglass::coreaudio;

TEST_CASE("Core Audio tap rejects missing UID or epoch without starting", "[coreaudio]") {
  OutputTap tap;
  CHECK(tap.status().state == TapState::stopped);
  CHECK_FALSE(tap.start("", 1));
  CHECK(tap.status().state == TapState::error);
  CHECK(tap.status().failure == TapFailure::invalid_argument);
  CHECK_FALSE(tap.start("test", 0));
  CHECK(tap.status().state == TapState::error);
  CHECK_FALSE(tap.start("org.beatglass.test.missing-output-device", 1));
  CHECK(tap.status().state == TapState::error);
  CHECK(tap.status().failure == TapFailure::not_found);
  tap.stop();
  CHECK(tap.status().state == TapState::stopped);
  CaptureBlock block;
  CHECK_FALSE(tap.pop(block));
}

TEST_CASE("Core Audio classifies only explicit permission errors as denial", "[coreaudio]") {
  CHECK(classify_osstatus(kAudioDevicePermissionsError) == TapFailure::permission_denied);
  CHECK(classify_osstatus(kAudioDeviceUnsupportedFormatError) == TapFailure::unsupported);
  CHECK(classify_osstatus(kAudioHardwareBadDeviceError) == TapFailure::device_lost);
  CHECK(classify_osstatus(kAudioHardwareUnspecifiedError) == TapFailure::backend_error);
  CHECK(classify_osstatus(0) == TapFailure::backend_error);
}
