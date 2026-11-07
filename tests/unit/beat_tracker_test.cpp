#include "../../lib/core/beat_tracker.hpp"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace beatglass::detail;
namespace {
std::vector<float> clicks(unsigned rate) {
  std::vector<float> audio(rate * 20);
  for (std::size_t i=0; i<audio.size(); ++i) {
    auto phase = i % (rate / 2);
    if (phase < 1000) audio[i] = static_cast<float>(.8 * std::exp(-double(phase)/100) * std::sin(double(phase)*.4));
  }
  return audio;
}
}
TEST_CASE("aubio beat events are causal and invariant to chunk size") {
  for (unsigned rate : {44100U, 48000U}) {
    auto audio = clicks(rate); BeatTracker whole(rate), chunked(rate);
    std::vector<BeatEvent> a, b;
    whole.process(audio, 9, 100, [&](const BeatEvent& e) { a.push_back(e); });
    for (std::size_t i=0; i<audio.size(); i+=137)
      chunked.process(std::span(audio).subspan(i, std::min<std::size_t>(137, audio.size()-i)), 9, 100+i,
                      [&](const BeatEvent& e) { b.push_back(e); });
    REQUIRE(a.size() >= 3); REQUIRE(a.size() == b.size());
    std::size_t plausible_intervals = 0;
    for (std::size_t i=0; i<a.size(); ++i) {
      CHECK(a[i].epoch == 9); CHECK(a[i].frame >= 100); CHECK(a[i].frame < a[i].observed_through);
      CHECK(a[i].observed_through <= 100 + audio.size());
      CHECK(a[i].frame == b[i].frame); CHECK(a[i].observed_through == b[i].observed_through);
      if (i) {
        REQUIRE(a[i-1].frame < a[i].frame);
        const double seconds = double(a[i].frame - a[i-1].frame) / rate;
        if (std::abs(seconds - .25) < .05 || std::abs(seconds - .5) < .05 || std::abs(seconds - 1.) < .1) ++plausible_intervals;
      }
    }
    CHECK(plausible_intervals >= 2);
  }
}
TEST_CASE("aubio beat events reacquire on gaps and reject invalid input") {
  auto audio = clicks(44100); BeatTracker tracker(44100); std::vector<BeatEvent> events;
  auto collect = [&](const BeatEvent& e) { events.push_back(e); };
  tracker.process(audio, 1, 0, collect); REQUIRE_FALSE(events.empty()); events.clear();
  tracker.process(std::span(audio).first(4096), 2, 0, collect); CHECK(events.empty());
  tracker.process(std::span(audio).first(4096), 2, 8192, collect); CHECK(events.empty());
  CHECK_THROWS(BeatTracker(0));
  const std::vector<float> invalid{std::numeric_limits<float>::quiet_NaN()};
  CHECK_THROWS(tracker.process(invalid, 2, 12288, collect));
  CHECK_THROWS(tracker.process(std::span(audio).first(10), 3, 0, collect));
}
TEST_CASE("observer failures invalidate the aubio tracker") {
  auto audio = clicks(44100); BeatTracker tracker(44100);
  CHECK_THROWS(tracker.process(audio, 1, 0, [](const BeatEvent&) { throw std::runtime_error("observer"); }));
  CHECK_THROWS(tracker.process(std::span(audio).first(1), 2, 0));
}
