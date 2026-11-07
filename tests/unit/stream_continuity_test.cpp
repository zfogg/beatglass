#include <catch2/catch_test_macros.hpp>
#include "beatglass/core/stream_continuity.hpp"
#include <array>
#include <limits>
using namespace beatglass;
TEST_CASE("stream transitions reacquire on gaps and epoch changes") {
 StreamContinuity stream; std::array<float,4> samples{};
 AudioBlock block{{48000,2}, 1, 0, 0, false, samples};
 CHECK(stream.observe(block) == StreamTransition::acquire);
 block.first_frame = 2; CHECK(stream.observe(block) == StreamTransition::contiguous);
 block.first_frame = 6; CHECK(stream.observe(block) == StreamTransition::reacquire);
 block.first_frame = 8; block.discontinuity = true; CHECK(stream.observe(block) == StreamTransition::reacquire);
 block.discontinuity = false; block.epoch = 2; block.first_frame = 0; block.format.sample_rate = 44100;
 CHECK(stream.observe(block) == StreamTransition::reacquire);
 stream.stop(); CHECK(stream.observe(block) == StreamTransition::acquire);
}
TEST_CASE("invalid blocks do not advance stream position") {
 StreamContinuity stream; std::array<float,4> samples{};
 AudioBlock block{{48000,2}, 1, 0, 0, false, samples}; REQUIRE(stream.observe(block) == StreamTransition::acquire);
 block.first_frame = 2; block.format.sample_rate = 44100; CHECK(stream.observe(block) == StreamTransition::invalid);
 block.format.sample_rate = 48000; CHECK(stream.observe(block) == StreamTransition::contiguous);
 block.first_frame = std::numeric_limits<std::uint64_t>::max(); CHECK(stream.observe(block) == StreamTransition::invalid);
 block.first_frame = 4; CHECK(stream.observe(block) == StreamTransition::contiguous);
}
