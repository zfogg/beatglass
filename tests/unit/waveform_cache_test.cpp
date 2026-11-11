#include "beatglass/core/waveform_cache.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <limits>

using namespace beatglass;

TEST_CASE("waveform extraction preserves source extrema across zoom buckets") {
  const std::array<float, 8> samples{-1.f, -.5f, 0.f, .25f, .5f, 1.f, .75f, -.25f};
  const auto envelope = extract_waveform_envelope(samples, 2);
  REQUIRE(envelope.size() == 2);
  CHECK(envelope[0].minimum == -1.f);
  CHECK(envelope[0].maximum == Catch::Approx(.25f));
  CHECK(envelope[1].minimum == Catch::Approx(-.25f));
  CHECK(envelope[1].maximum == 1.f);
}

TEST_CASE("waveform cache separates zoom and source revisions and invalidates") {
  WaveformCache cache(4);
  const std::array<float, 8> samples{};
  const auto& zoom_two = cache.get("song", 1, 2, samples);
  const auto& zoom_four = cache.get("song", 1, 4, samples);
  REQUIRE(zoom_two.size() == 2);
  REQUIRE(zoom_four.size() == 4);
  CHECK(cache.size() == 2);
  const auto& hit = cache.get("song", 1, 2, samples);
  CHECK(&hit == &zoom_two);
  cache.get("song", 2, 2, samples);
  CHECK(cache.size() == 3);
  cache.invalidate("song");
  CHECK(cache.size() == 0);
}

TEST_CASE("waveform extraction rejects invalid source PCM and zoom") {
  const std::array<float, 4> samples{};
  CHECK_THROWS(extract_waveform_envelope(samples, 0));
  CHECK_THROWS(extract_waveform_envelope(samples, 5));
  auto invalid = samples;
  invalid[2] = std::numeric_limits<float>::quiet_NaN();
  CHECK_THROWS(extract_waveform_envelope(invalid, 2));
}
