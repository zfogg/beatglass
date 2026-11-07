#include <catch2/catch_test_macros.hpp>
#include "beatglass/core/tracking_machine.hpp"
using namespace beatglass;
TEST_CASE("tracking lifecycle rejects old results across silence and resume") {
 TrackingMachine m; CHECK(m.state() == TrackingState::stopped); CHECK_FALSE(m.accept({1,1}));
 REQUIRE(m.start(1)); auto old = m.token(); CHECK_FALSE(m.start(1));
 REQUIRE(m.accept(old)); CHECK(m.state() == TrackingState::tracking);
 REQUIRE(m.silence()); CHECK(m.state() == TrackingState::silent); CHECK_FALSE(m.accept(old));
 REQUIRE(m.signal()); CHECK(m.state() == TrackingState::acquiring); CHECK_FALSE(m.accept(old));
 REQUIRE(m.accept(m.token())); CHECK(m.state() == TrackingState::tracking);
}
TEST_CASE("discontinuity, expiry, error and restart invalidate in-flight evidence") {
 TrackingMachine m; REQUIRE(m.start(3)); auto old = m.token();
 REQUIRE(m.discontinuity(4)); CHECK(m.state() == TrackingState::reacquiring); CHECK_FALSE(m.accept(old));
 auto current = m.token(); REQUIRE(m.accept(current)); REQUIRE(m.expire()); CHECK_FALSE(m.accept(current));
 current = m.token(); m.fail(); CHECK(m.state() == TrackingState::error); CHECK_FALSE(m.accept(current));
 REQUIRE(m.start(4)); CHECK_FALSE(m.accept(current)); current = m.token(); m.stop();
 CHECK_FALSE(m.accept(current)); REQUIRE(m.start(4)); CHECK_FALSE(m.accept(current));
}
TEST_CASE("inactive and silent states cannot accept estimates or expire") {
 TrackingMachine m; CHECK_FALSE(m.signal()); CHECK_FALSE(m.silence()); CHECK_FALSE(m.expire()); CHECK_FALSE(m.discontinuity(2));
 REQUIRE(m.start(2)); REQUIRE(m.silence()); const auto t = m.token(); REQUIRE(m.silence());
 CHECK(m.token().generation == t.generation); CHECK_FALSE(m.expire()); CHECK_FALSE(m.accept(t));
}
