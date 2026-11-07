#include <catch2/catch_test_macros.hpp>
#include "beatglass/core/audio_queue.hpp"
#include <array>
using namespace beatglass;
TEST_CASE("queue preserves data and wraps") {
  AudioQueue<3, 4> q; AudioQueue<3, 4>::Block out;
  std::array<float, 4> samples{1, 2, 3, 4};
  AudioBlock in{{48000, 2}, 7, 10, 123, false, samples};
  REQUIRE(q.push(in)); samples[0] = 9;
  REQUIRE(q.pop(out)); CHECK(out.samples[0] == 1); CHECK(out.first_frame == 10);
  REQUIRE(q.push(in)); REQUIRE(q.pop(out)); REQUIRE(q.push(in)); REQUIRE(q.pop(out));
  CHECK_FALSE(q.pop(out));
}
TEST_CASE("overflow drops complete blocks and marks the next block") {
  AudioQueue<2, 4> q; AudioQueue<2, 4>::Block out;
  std::array<float, 4> samples{}; AudioBlock in{{44100, 2}, 1, 0, 0, false, samples};
  REQUIRE(q.push(in)); CHECK_FALSE(q.push(in)); CHECK(q.dropped_frames() == 2);
  REQUIRE(q.pop(out)); CHECK_FALSE(out.discontinuity);
  in.first_frame = 4; REQUIRE(q.push(in)); REQUIRE(q.pop(out));
  CHECK(out.discontinuity); CHECK(out.first_frame == 4);
}
TEST_CASE("malformed and oversized blocks cannot enter queue") {
  AudioQueue<2, 2> q; AudioQueue<2, 2>::Block out;
  std::array<float, 3> samples{}; AudioBlock in{{48000, 2}, 1, 0, 0, false, samples};
  CHECK_FALSE(q.push(in)); CHECK_FALSE(q.pop(out));
  std::array<float, 2> valid{}; in.samples = valid;
  REQUIRE(q.push(in)); REQUIRE(q.pop(out)); CHECK(out.discontinuity);
}
