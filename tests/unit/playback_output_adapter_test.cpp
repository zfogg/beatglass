#include <catch2/catch_test_macros.hpp>
#include "beatglass/capture/playback_output_adapter.hpp"
#include <array>
using namespace beatglass;
TEST_CASE("output adapter drains copied PCM") { PlaybackOutputAdapter a(2,4); std::array<float,2> in{.25f,.5f}; AudioBlock s{{44100,1},3,8,0,false,in}; REQUIRE(a.submit(s)); std::array<float,4> out{}; AudioBlock g{}; REQUIRE(a.render_one(out,[&](const AudioBlock& b){g=b;})); CHECK(g.epoch==3); CHECK(g.samples.data()==out.data()); CHECK(out[1]==.5f); CHECK(a.queued()==0); }
TEST_CASE("output adapter consumes failed device write") { PlaybackOutputAdapter a(1,2); std::array<float,1> in{1}; AudioBlock s{{8000,1},1,0,0,false,in}; REQUIRE(a.submit(s)); std::array<float,2> out{}; CHECK_FALSE(a.render_one(out,[](const AudioBlock&){throw 1;})); CHECK(a.queued()==0); }
TEST_CASE("output adapter keeps queued PCM when writer is absent") { PlaybackOutputAdapter a(1,2); std::array<float,1> in{1}; AudioBlock s{{8000,1},1,0,0,false,in}; REQUIRE(a.submit(s)); std::array<float,1> out{}; CHECK_FALSE(a.render_one(out,{})); CHECK(a.queued()==1); }
