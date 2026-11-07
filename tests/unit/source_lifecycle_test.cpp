#include <catch2/catch_test_macros.hpp>
#include "beatglass/capture/source_lifecycle.hpp"
#include "beatglass/core/snapshot_publisher.hpp"
using namespace beatglass;

TEST_CASE("source interruption invalidates the epoch until a fresh epoch resumes") {
  SourceLifecycle m;
  CHECK(m.state() == SourceState::stopped); CHECK_FALSE(m.epoch());
  auto t = m.start(); REQUIRE(t); CHECK_FALSE(m.start());
  CHECK_FALSE(m.interrupt(*t)); REQUIRE(m.running(*t, 10)); CHECK(*m.epoch() == 10);
  REQUIRE(m.interrupt(*t)); CHECK_FALSE(m.epoch());
  CHECK_FALSE(m.running(*t, 10)); CHECK_FALSE(m.running(*t, 9)); CHECK(m.state() == SourceState::interrupted);
  REQUIRE(m.running(*t, 11)); CHECK(*m.epoch() == 11);
  CHECK_FALSE(m.running(*t, 12));
}
TEST_CASE("late source notifications cannot revive a stopped or restarted source") {
  SourceLifecycle m; auto old = m.start(); REQUIRE(old); REQUIRE(m.running(*old, 1));
  m.stop(); CHECK_FALSE(m.running(*old, 2)); CHECK_FALSE(m.fail(*old));
  auto current = m.start(); REQUIRE(current); CHECK(current->generation != old->generation);
  CHECK_FALSE(m.running(*old, 3)); CHECK_FALSE(m.fail(*old)); CHECK_FALSE(m.running(*current, 1)); REQUIRE(m.running(*current, 3));
  REQUIRE(m.fail(*current)); CHECK_FALSE(m.epoch()); CHECK_FALSE(m.interrupt(*current));
  auto retry = m.start(); REQUIRE(retry); CHECK_FALSE(m.running(*current, 4)); REQUIRE(m.running(*retry, 4));
}

TEST_CASE("interruption handoff clears publication and rejects in-flight results") {
  SourceLifecycle source; TrackingMachine tracking; SnapshotPublisher publication(2, 2);
  auto attempt = source.start(); REQUIRE(attempt); REQUIRE(source.running(*attempt, 41));
  REQUIRE(tracking.start(*source.epoch())); auto old = tracking.token();
  publication.begin_acquisition(old); Snapshot first; first.observed_through = {41, 10};
  REQUIRE(tracking.accept(old)); REQUIRE(publication.publish(old, first)); REQUIRE(publication.latest());
  REQUIRE(source.interrupt(*attempt)); tracking.stop(); publication.clear();
  CHECK_FALSE(publication.latest()); CHECK_FALSE(tracking.accept(old)); CHECK_FALSE(publication.publish(old, first));
  REQUIRE(source.running(*attempt, 42)); REQUIRE(tracking.start(*source.epoch()));
  auto next = tracking.token(); publication.begin_acquisition(next);
  CHECK_FALSE(tracking.accept(old)); CHECK_FALSE(publication.publish(old, first));
  Snapshot fresh; fresh.observed_through = {42, 5}; REQUIRE(tracking.accept(next));
  REQUIRE(publication.publish(next, fresh)); CHECK(publication.latest()->observed_through.epoch == 42);
}
