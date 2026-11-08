#include "../../lib/core/tonal_tracker.hpp"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <vector>
using beatglass::detail::TonalTracker;
static std::vector<float> triad(double a, double b, double c, std::size_t n) {
  std::vector<float> pcm(n);
  for (std::size_t i = 0; i < n; ++i) {
    const auto t = 2 * std::numbers::pi * i / 44100.;
    pcm[i] = static_cast<float>((std::sin(a*t) + std::sin(b*t) + std::sin(c*t)) / 3);
  }
  return pcm;
}
TEST_CASE("tonal tracker detects generated triads and is independent of chunk sizes") {
  auto pcm = triad(261.625565, 329.627557, 391.995436, 44100);
  TonalTracker whole(44100), chunks(44100), prefix(44100);
  prefix.process(std::span(pcm).first(8192), 3, 0);
  chunks.process(std::span(pcm).first(8192), 3, 0);
  CHECK(prefix.estimate().chord == chunks.estimate().chord);
  CHECK(prefix.estimate().observed_through == 8192);
  chunks.process(std::span(pcm).subspan(8192), 3, 8192);
  TonalTracker arbitrary(44100);
  whole.process(pcm, 3, 0);
  for (std::size_t i = 0; i < pcm.size(); i += 137)
    arbitrary.process(std::span(pcm).subspan(i, std::min<std::size_t>(137, pcm.size()-i)), 3, i);
  const auto a = whole.estimate(), b = arbitrary.estimate();
  REQUIRE(a.chord); CHECK(*a.chord == "C major"); CHECK(a.key); CHECK(a.chord == b.chord);
  CHECK(a.key_strength > 0.0f); CHECK(a.chord_strength > 0.0f);
  CHECK(a.key == b.key); CHECK(a.observed_through == b.observed_through); CHECK(a.observed_through <= pcm.size());
  auto minor = triad(220, 261.625565, 329.627557, 4096);
  chunks.process(minor, 4, 0); REQUIRE(chunks.estimate().chord); CHECK(*chunks.estimate().chord == "A minor");
}
TEST_CASE("tonal tracker clears stale evidence on gaps epochs and silence") {
  auto pcm = triad(261.625565, 329.627557, 391.995436, 4096);
  TonalTracker t(44100); t.process(pcm, 1, 0); REQUIRE(t.estimate().chord);
  t.process(std::span(pcm).first(100), 1, 5000); CHECK_FALSE(t.estimate().chord);
  t.process(pcm, 2, 0); REQUIRE(t.estimate().chord);
  std::vector<float> silence(8192); t.process(silence, 2, pcm.size()); CHECK_FALSE(t.estimate().chord); CHECK_FALSE(t.estimate().key);
  t.process(std::span(pcm).first(100), 2, pcm.size()+silence.size(), true); CHECK_FALSE(t.estimate().chord);
  CHECK_THROWS(TonalTracker(8000));
  t.process(pcm, 3, 0); REQUIRE(t.estimate().chord);
  const std::vector<float> bad{std::numeric_limits<float>::quiet_NaN()};
  CHECK_THROWS(t.process(bad, 2, 0)); CHECK_FALSE(t.estimate().chord);
}
