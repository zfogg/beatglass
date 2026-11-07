#include <catch2/catch_test_macros.hpp>
#include "beatglass/core/tempo_tracker.hpp"
#include <cmath>
#include <vector>
using namespace beatglass;
TEST_CASE("tempo consumes arbitrary contiguous chunks without future samples") {
 std::vector<float> audio(44100 * 20);
 for (std::size_t i = 0; i < audio.size(); ++i) { auto phase = i % 22050; if (phase < 1000) audio[i] = float(.8 * std::exp(-double(phase)/100) * std::sin(double(phase)*.4)); }
 TempoTracker whole(44100), chunked(44100);
 whole.process(audio, 1, 0);
 for (std::size_t i=0; i<audio.size(); i+=137) chunked.process(std::span(audio).subspan(i, std::min<std::size_t>(137, audio.size()-i)), 1, i);
 REQUIRE(whole.estimate().bpm); REQUIRE(chunked.estimate().bpm);
 CHECK(whole.estimate().bpm == chunked.estimate().bpm); CHECK(whole.estimate().beats == chunked.estimate().beats);
 std::vector<float> silence(4096);
 chunked.process(silence, 1, audio.size()+2); CHECK_FALSE(chunked.estimate().bpm); CHECK(chunked.estimate().beats == 0);
 whole.process(silence, 2, 0); CHECK_FALSE(whole.estimate().bpm);
 whole.process(audio, 3, 0); REQUIRE(whole.estimate().bpm);
 whole.process(silence, 3, audio.size(), true); CHECK_FALSE(whole.estimate().bpm); CHECK(whole.estimate().beats == 0);
}
TEST_CASE("invalid tempo sample rate is rejected") { CHECK_THROWS(TempoTracker(0)); }

#include "../../src/cli/state_label.hpp"
#include <string>
TEST_CASE("CLI state labels preserve errors and unknown enum values") {
  using beatglass::cli::state_label;
  CHECK(std::string(state_label(TrackingState::error)) == "error");
  CHECK(std::string(state_label(static_cast<TrackingState>(999))) == "unknown");
  CHECK(std::string(state_label(TrackingState::acquiring)) == "acquiring");
}
