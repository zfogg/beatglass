#include <catch2/catch_test_macros.hpp>
#include "beatglass/capture/source_lifecycle.hpp"
#include "beatglass/core/snapshot_publisher.hpp"
using namespace beatglass;
TEST_CASE("ordered source events reject duplicates and fail closed on gaps") {
 SourceLifecycle s; auto a = s.start(); REQUIRE(a);
 REQUIRE(s.notify(*a, 1, SourceState::running, 10) == SourceEventResult::accepted);
 CHECK(s.notify(*a, 1, SourceState::interrupted) == SourceEventResult::stale);
 REQUIRE(s.epoch() == 10);
 CHECK(s.notify(*a, 3, SourceState::interrupted) == SourceEventResult::sequence_gap);
 CHECK(s.state() == SourceState::error); CHECK_FALSE(s.epoch());
 CHECK(s.notify(*a, 2, SourceState::running, 11) == SourceEventResult::stale);
}
TEST_CASE("interruption requires a new epoch and old attempts cannot revive") {
 SourceLifecycle s; auto a = s.start(); REQUIRE(a);
 REQUIRE(s.notify(*a, 1, SourceState::running, 10) == SourceEventResult::accepted);
 REQUIRE(s.notify(*a, 2, SourceState::interrupted) == SourceEventResult::accepted);
 CHECK_FALSE(s.epoch());
 CHECK(s.notify(*a, 3, SourceState::running, 10) == SourceEventResult::invalid);
 CHECK(s.state() == SourceState::error);
 auto b = s.start(); REQUIRE(b);
 CHECK(s.notify(*a, 4, SourceState::running, 11) == SourceEventResult::stale);
 REQUIRE(s.notify(*b, 1, SourceState::running, 11) == SourceEventResult::accepted);
 s.delivery_lost(); CHECK_FALSE(s.epoch()); CHECK(s.state() == SourceState::error);
 s.stop(); CHECK(s.notify(*b, 2, SourceState::running, 12) == SourceEventResult::stale);
}

TEST_CASE("epoch allocator persists across source instances") {
 SourceEpochAllocator epochs; auto first = epochs.next(); auto second = epochs.next();
 REQUIRE(first); REQUIRE(second); CHECK(*second > *first);
 SourceLifecycle a; auto attempt = a.start(); REQUIRE(attempt);
 REQUIRE(a.notify(*attempt, 1, SourceState::running, *first) == SourceEventResult::accepted);
 a.stop(); SourceLifecycle b; auto next = b.start(); REQUIRE(next);
 REQUIRE(b.notify(*next, 1, SourceState::running, *second) == SourceEventResult::accepted);
}

TEST_CASE("source interruption invalidates in-flight publication") {
 SourceLifecycle source; TrackingMachine tracking; SnapshotPublisher publication(2, 2);
 auto attempt = source.start(); REQUIRE(attempt);
 REQUIRE(source.notify(*attempt, 1, SourceState::running, 41) == SourceEventResult::accepted);
 REQUIRE(tracking.start(*source.epoch())); auto old = tracking.token();
 publication.begin_acquisition(old); Snapshot first; first.observed_through = {41, 10};
 REQUIRE(publication.publish(old, first));
 REQUIRE(source.notify(*attempt, 2, SourceState::interrupted) == SourceEventResult::accepted);
 tracking.stop(); publication.clear(); CHECK_FALSE(tracking.accept(old)); CHECK_FALSE(publication.publish(old, first));
 REQUIRE(source.notify(*attempt, 3, SourceState::running, 42) == SourceEventResult::accepted);
 REQUIRE(tracking.start(*source.epoch())); publication.begin_acquisition(tracking.token());
 CHECK_FALSE(publication.publish(old, first));
}
