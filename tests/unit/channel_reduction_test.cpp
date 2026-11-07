#include <catch2/catch_test_macros.hpp>
#include "beatglass/core/channel_reduction.hpp"
#include <array>
#include <limits>
using namespace beatglass;
TEST_CASE("reduction preserves anti-phase stereo instead of cancelling it") {
 std::array<float, 6> input{.5f,-.5f,-.25f,.25f,.75f,-.75f}; std::array<float,3> output{};
 REQUIRE(reduce_loudest_channel(input, 2, output));
 CHECK((output == std::array<float,3>{.5f,-.25f,.75f}));
}
TEST_CASE("reduction chooses loudest channel and rejects malformed PCM without modifying output") {
 std::array<float,4> input{.1f,.9f,.2f,.8f}; std::array<float,2> output{};
 REQUIRE(reduce_loudest_channel(input,2,output)); CHECK((output == std::array<float,2>{.9f,.8f}));
 auto before = output; CHECK_FALSE(reduce_loudest_channel(input,3,output)); CHECK(output == before);
 input[1] = std::numeric_limits<float>::quiet_NaN(); CHECK_FALSE(reduce_loudest_channel(input,2,output)); CHECK(output == before);
}
