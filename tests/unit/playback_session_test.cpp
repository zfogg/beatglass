#include <catch2/catch_test_macros.hpp>
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
TEST_CASE("playback advances by decoded block frame count") {
  PlaybackSession session; REQUIRE(session.start(11));
  REQUIRE(session.accept(11, 0, 512)); CHECK(session.next_frame() == 512);
  REQUIRE(session.accept(11, 512, 256)); CHECK(session.next_frame() == 768);
  CHECK_FALSE(session.accept(11, 768, 0));
}
