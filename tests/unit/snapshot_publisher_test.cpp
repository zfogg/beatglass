#include <catch2/catch_test_macros.hpp>
#include "beatglass/core/snapshot_publisher.hpp"
#include <atomic>
#include <thread>
using namespace beatglass;
TEST_CASE("snapshot publication bounds histories and keeps reader copies immutable") {
 SnapshotPublisher p(2, 1); Snapshot s; s.observed_through = {7, 100};
 s.beats = {{{7,10},1}, {{7,20},1}, {{7,30},1}};
 s.key_candidates = {{"A",1}, {"B",2}};
 CHECK_FALSE(p.publish({7,1}, s)); p.begin_acquisition({7,1}); REQUIRE(p.publish({7,1}, s));
 auto old = p.latest(); REQUIRE(old); REQUIRE(old->beats.size() == 2); CHECK(old->beats[0].time.frame == 20);
 REQUIRE(old->key_candidates.size() == 1); CHECK(old->key_candidates[0].label == "B");
 s.observed_through.frame = 101; REQUIRE(p.publish({7,1}, s)); CHECK(old->observed_through.frame == 100);
 p.begin_acquisition({7,2}); CHECK_FALSE(p.publish({7,1}, s));
 p.begin_acquisition({8,3}); CHECK_FALSE(p.latest()); CHECK(old->observed_through.frame == 100);
 CHECK_FALSE(p.publish({7,1}, s));
 p.begin_acquisition({7,4}); REQUIRE(p.publish({7,4}, s)); p.clear();
 CHECK_FALSE(p.latest()); CHECK_FALSE(p.publish({7,4}, s));
}
TEST_CASE("snapshot publisher rejects stale and invalid event timelines") {
 SnapshotPublisher p(3,3); p.begin_acquisition({2,1}); Snapshot s; s.observed_through = {2,10}; REQUIRE(p.publish({2,1}, s));
 s.observed_through.frame = 9; CHECK_FALSE(p.publish({2,1}, s)); CHECK(p.latest()->observed_through.frame == 10);
 s.observed_through.frame = 11; s.beats = {{{2,12},1}}; CHECK_FALSE(p.publish({2,1}, s));
 s.beats.clear(); s.chords = {{{2,2},{2,4},{"C",1}}, {{2,3},{2,5},{"G",1}}}; CHECK_FALSE(p.publish({2,1}, s));
 s.chords.clear(); s.key_candidates = {{std::string(129, 'x'), 1}}; CHECK_FALSE(p.publish({2,1}, s));
}
TEST_CASE("snapshot publisher supports concurrent readers") {
 SnapshotPublisher p(2,2); p.begin_acquisition({1,1}); std::atomic<bool> done{false}, bad{false};
 std::thread reader([&] { while (!done.load()) { auto s = p.latest(); if (s && (s->observed_through.epoch != 1 || s->beats.size() > 2)) bad.store(true); } });
 bool published = true;
 for (std::uint64_t i=0; i<10000; ++i) { Snapshot s; s.observed_through = {1,i}; if (!p.publish({1,1}, std::move(s))) published = false; }
 done.store(true); reader.join(); CHECK(published); CHECK_FALSE(bad.load());
}
TEST_CASE("snapshot timelines use exclusive observed and half-open chord boundaries") {
 SnapshotPublisher p(3,3); p.begin_acquisition({2,1}); Snapshot s; s.observed_through = {2,10};
 s.beats = {{{2,9},1}}; s.chords = {{{2,0},{2,10},{"C",1}}}; REQUIRE(p.publish({2,1}, s));
 s.beats[0].time.frame = 10; CHECK_FALSE(p.publish({2,1}, s));
 s.beats.clear(); s.chords[0].start.frame = 10; CHECK_FALSE(p.publish({2,1}, s));
 CHECK(p.latest()->chords[0].start.frame == 0);
 s.observed_through.frame = 0; s.chords.clear(); s.beats = {{{2,0},1}};
 SnapshotPublisher empty(3,3); empty.begin_acquisition({2,1}); CHECK_FALSE(empty.publish({2,1}, s));
}
