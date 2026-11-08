#include <catch2/catch_test_macros.hpp>
#include <limits>
#include "beatglass/core/playback_session.hpp"
using namespace beatglass;
TEST_CASE("playback seek retires old epoch and reacquires from requested frame") {
  PlaybackSession session;
  CHECK_FALSE(session.start(0)); REQUIRE(session.start(7));
  REQUIRE(session.accept(7, 0)); CHECK_FALSE(session.accept(6, 1));
  REQUIRE(session.seek(42, 8)); CHECK(session.next_frame() == 42);
  CHECK_FALSE(session.accept(7, 42)); REQUIRE(session.accept(8, 42));
}
TEST_CASE("paused playback resumes without retiring its epoch") {
  PlaybackSession session; REQUIRE(session.start(3)); REQUIRE(session.pause());
  CHECK_FALSE(session.accept(3, 0)); REQUIRE(session.resume()); REQUIRE(session.accept(3, 0));
}
TEST_CASE("source replacement resets the queue cursor and retires old deliveries") {
  PlaybackSession session; REQUIRE(session.start(4)); REQUIRE(session.accept(4, 0));
  REQUIRE(session.replace(5)); CHECK(session.next_frame() == 0);
  CHECK_FALSE(session.accept(4, 0)); REQUIRE(session.accept(5, 0));
  CHECK_FALSE(session.replace(5));
}
TEST_CASE("source replacement reacquires a paused session for the new source") {
  PlaybackSession session; REQUIRE(session.start(9)); REQUIRE(session.pause());
  REQUIRE(session.replace(10));
  CHECK(session.state() == PlaybackState::playing);
  CHECK_FALSE(session.accept(9, 0)); REQUIRE(session.accept(10, 0));
}
TEST_CASE("stop retires source epoch and blocks stale or resumed delivery") {
  PlaybackSession session; REQUIRE(session.start(14)); REQUIRE(session.accept(14, 0, 128));
  session.stop();
  CHECK(session.state() == PlaybackState::stopped);
  CHECK(session.epoch() == 0); CHECK(session.next_frame() == 0);
  CHECK_FALSE(session.accept(14, 128));
  CHECK_FALSE(session.pause()); CHECK_FALSE(session.resume());
  REQUIRE(session.start(15)); CHECK(session.state() == PlaybackState::playing);
  CHECK_FALSE(session.accept(14, 0)); REQUIRE(session.accept(15, 0));
}
TEST_CASE("pause rejects delivery until resume and preserves source cursor") {
  PlaybackSession session; REQUIRE(session.start(16)); REQUIRE(session.accept(16, 0, 64));
  REQUIRE(session.pause()); CHECK(session.next_frame() == 64);
  CHECK_FALSE(session.accept(16, 64)); CHECK_FALSE(session.pause());
  REQUIRE(session.resume()); REQUIRE(session.accept(16, 64, 32));
  CHECK(session.next_frame() == 96);
}
TEST_CASE("playback advances by decoded block frame count") {
  PlaybackSession session; REQUIRE(session.start(11));
  REQUIRE(session.accept(11, 0, 512)); CHECK(session.next_frame() == 512);
  REQUIRE(session.accept(11, 512, 256)); CHECK(session.next_frame() == 768);
  CHECK_FALSE(session.accept(11, 768, 0));
}
TEST_CASE("playback rejects frame cursor overflow without advancing") {
  PlaybackSession session; REQUIRE(session.start(12));
  REQUIRE(session.seek(std::numeric_limits<std::uint64_t>::max() - 1, 13));
  CHECK_FALSE(session.accept(13, std::numeric_limits<std::uint64_t>::max() - 1, 2));
  CHECK(session.next_frame() == std::numeric_limits<std::uint64_t>::max() - 1);
}
