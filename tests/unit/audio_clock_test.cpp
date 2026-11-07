#include <catch2/catch_test_macros.hpp>
#include "beatglass/core/audio_clock.hpp"
#include <limits>
using namespace beatglass;
TEST_CASE("frame positions map in both directions within an epoch") {
 AudioClockAnchor a{4, 48000, 10000000000};
 CHECK(frame_to_monotonic_ns(a, 4, 96000, 48000) == 11000000000LL);
 CHECK(frame_to_monotonic_ns(a, 4, 0, 48000) == 9000000000LL);
 CHECK(frame_to_monotonic_ns(a, 4, 48001, 48000) == 10000020833LL);
 CHECK_FALSE(frame_to_monotonic_ns(a, 5, 48000, 48000));
 CHECK_FALSE(frame_to_monotonic_ns(a, 4, 48000, 0));
}
TEST_CASE("clock mapping refuses arithmetic overflow") {
 using L = std::numeric_limits<std::int64_t>;
 CHECK_FALSE(frame_to_monotonic_ns({1, 0, L::max()}, 1, 1, 48000));
 CHECK_FALSE(frame_to_monotonic_ns({1, 1, L::min()}, 1, 0, 48000));
 CHECK_FALSE(frame_to_monotonic_ns({1, 0, 0}, 1, std::numeric_limits<std::uint64_t>::max(), 1));
}
