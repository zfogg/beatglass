#include <catch2/catch_test_macros.hpp>
#include "beatglass/capture/playback_pcm_queue.hpp"
#include <array>
#include <limits>
using namespace beatglass;
TEST_CASE("playback queue rejects malformed audio format") {
  PlaybackPcmQueue q(2, 2); std::array<float,1> samples{1};
  AudioBlock source{{0,1},1,0,0,false,samples}; CHECK_FALSE(q.push(source)); CHECK(q.size()==0);
}
TEST_CASE("playback queue rejects blocks without a source epoch") {
  PlaybackPcmQueue q(2, 2); std::array<float,1> samples{1};
  AudioBlock source{{44100,1},0,0,0,false,samples}; CHECK_FALSE(q.push(source)); CHECK(q.size()==0);
}
TEST_CASE("playback queue copies block metadata and samples") {
  PlaybackPcmQueue q(2, 4); std::array<float,4> in{.1f,.2f,.3f,.4f}; AudioBlock source{{48000,2},9,12,77,true,in};
  REQUIRE(q.push(source)); REQUIRE(q.size()==1); std::array<float,4> out{}; AudioBlock got{};
  REQUIRE(q.pop(out,got)); CHECK(got.epoch==9); CHECK(got.first_frame==12); CHECK(got.discontinuity); CHECK(got.samples.size()==4); CHECK(out[2]==.3f); CHECK(out[3]==.4f); CHECK(q.size()==0);
}
TEST_CASE("playback queue rejects oversize and full blocks without allocation") {
  PlaybackPcmQueue q(1,2); std::array<float,3> large{}; AudioBlock too_large{{44100,1},1,0,0,false,large}; CHECK_FALSE(q.push(too_large));
  std::array<float,2> data{1,2}; AudioBlock block{{44100,1},1,0,0,false,data}; REQUIRE(q.push(block)); CHECK_FALSE(q.push(block)); std::array<float,1> short_out{}; AudioBlock got{}; CHECK_FALSE(q.pop(short_out,got)); CHECK(q.size()==1); std::array<float,2> out{}; REQUIRE(q.pop(out,got));
}
TEST_CASE("playback queue requires bounded dimensions") { CHECK_THROWS(PlaybackPcmQueue(0,1)); CHECK_THROWS(PlaybackPcmQueue(1,0)); CHECK_THROWS(PlaybackPcmQueue(std::numeric_limits<std::size_t>::max(),1)); }
