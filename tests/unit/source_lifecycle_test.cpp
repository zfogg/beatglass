#include <catch2/catch_test_macros.hpp>
#include "beatglass/capture/source_lifecycle.hpp"
#include "beatglass/core/snapshot_publisher.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <thread>
#include <vector>
using namespace beatglass;
TEST_CASE("source lifecycle rejects zero running epochs") {
  SourceLifecycle lifecycle; auto attempt = lifecycle.start(); REQUIRE(attempt);
  CHECK(lifecycle.notify(*attempt, 1, SourceState::running, 0) == SourceEventResult::invalid);
  CHECK(lifecycle.state() == SourceState::error);
}
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

TEST_CASE("shared epoch allocator issues unique values to concurrent workers") {
 SourceEpochAllocator epochs; constexpr std::size_t workers = 8, count = 2000;
 std::array<std::vector<std::uint64_t>, workers> issued; std::array<std::thread, workers> threads;
 for (std::size_t i = 0; i < workers; ++i) threads[i] = std::thread([&, i] {
   issued[i].reserve(count);
   for (std::size_t j = 0; j < count; ++j) { auto epoch = epochs.next(); if (epoch) issued[i].push_back(*epoch); }
 });
 for (auto& t : threads) t.join();
 std::vector<std::uint64_t> all;
 for (const auto& batch : issued) { REQUIRE(batch.size() == count); all.insert(all.end(), batch.begin(), batch.end()); }
 std::sort(all.begin(), all.end()); REQUIRE(all.size() == workers * count);
 for (std::size_t i = 0; i < all.size(); ++i) CHECK(all[i] == i + 1);
}
TEST_CASE("shared epoch exhaustion never wraps under contention") {
 constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
 SourceEpochAllocator epochs(maximum - 2); std::array<std::optional<std::uint64_t>, 8> results;
 std::array<std::thread, 8> threads;
 for (std::size_t i = 0; i < threads.size(); ++i) threads[i] = std::thread([&, i] { results[i] = epochs.next(); });
 for (auto& t : threads) t.join();
 std::vector<std::uint64_t> issued; for (auto value : results) if (value) issued.push_back(*value);
 std::sort(issued.begin(), issued.end()); REQUIRE(issued.size() == 2);
 CHECK(issued[0] == maximum - 1); CHECK(issued[1] == maximum); CHECK_FALSE(epochs.next());
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

TEST_CASE("late notifications from a retired attempt cannot invalidate a newer run") {
 SourceLifecycle source; auto first = source.start(); REQUIRE(first);
 REQUIRE(source.notify(*first, 1, SourceState::running, 101) == SourceEventResult::accepted);
 REQUIRE(source.notify(*first, 2, SourceState::interrupted) == SourceEventResult::accepted);
 auto second = source.start(); REQUIRE(second);
 REQUIRE(source.notify(*second, 1, SourceState::running, 102) == SourceEventResult::accepted);

 // A delayed callback from the retired generation must be ignored, not fail
 // closed, so the currently monitored endpoint remains publishable.
 CHECK(source.notify(*first, 3, SourceState::interrupted) == SourceEventResult::stale);
 CHECK(source.state() == SourceState::running);
 REQUIRE(source.epoch());
 CHECK(*source.epoch() == 102);
}
