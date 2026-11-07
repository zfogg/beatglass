#include <catch2/catch_test_macros.hpp>
#include "library_bridge.hpp"
#include <chrono>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <mutex>
#include <thread>
using namespace beatglass::library_detail;
namespace {
struct Dispatcher {
  std::mutex mutex; std::condition_variable ready; std::deque<std::function<void()>> calls;
  void post(std::function<void()> call) { std::lock_guard lock(mutex); calls.push_back(std::move(call)); ready.notify_one(); }
  std::function<void()> take() { std::unique_lock lock(mutex); if (!ready.wait_for(lock, std::chrono::seconds(10), [&]{ return !calls.empty(); })) return {}; auto call = std::move(calls.front()); calls.pop_front(); return call; }
};
}
TEST_CASE("library bridge delivers owned results on dispatcher and suppresses stale work") {
  auto dir = std::filesystem::temp_directory_path() / ("beatglass-bridge-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directory(dir); auto db = dir / "cache.sqlite";
  { TempoAnalysisStore store(db.string()); CollectionEntry row; row.path = "track"; row.title = "Title"; store.put_entry(row); }
  Dispatcher queue; int delivered = 0; std::optional<LibraryBridge::Result> result; auto gui = std::this_thread::get_id();
  std::function<void()> after_destroy;
  {
    LibraryBridge bridge(db.string(), [&](auto call){ queue.post(std::move(call)); });
    bridge.entries({}, [&](auto value){ REQUIRE(std::this_thread::get_id() == gui); ++delivered; result = std::move(value); });
    auto first = queue.take(); REQUIRE(first);
    bridge.detail("track", [&](auto value){ REQUIRE(std::this_thread::get_id() == gui); ++delivered; result = std::move(value); });
    first(); REQUIRE(delivered == 0);
    auto second = queue.take(); REQUIRE(second); second(); REQUIRE(delivered == 1);
    REQUIRE(result); auto detail = std::get<LibraryBridge::Detail>(result->value); REQUIRE(detail); REQUIRE(detail->title == "Title");
    bridge.analyze("unused", "engine", "config", 0, [&](auto value){ result = std::move(value); ++delivered; });
    auto third = queue.take(); REQUIRE(third); third(); REQUIRE(delivered == 2); REQUIRE_FALSE(result->error.empty());
    auto audio = dir / "tone.aiff";
    std::filesystem::copy_file(std::filesystem::path(BEATGLASS_FIXTURES) / "tone.aiff", audio);
    bridge.analyze(audio, "bridge-test", "default", 42, [&](auto value){ result = std::move(value); ++delivered; });
    auto real = queue.take(); REQUIRE(real); real(); REQUIRE(delivered == 3);
    auto analysis = std::get<LibraryBridge::Analysis>(result->value); INFO(analysis.diagnostic);
    REQUIRE(analysis.status == FileJobStatus::committed); REQUIRE(analysis.value); REQUIRE(analysis.value->result.observed_through.epoch == 42);
    auto identity = analysis.value->identity;
    auto ticket = bridge.lookup(audio, identity, 43, [&](auto value){ result = std::move(value); ++delivered; });
    auto lookup_call = queue.take(); REQUIRE(lookup_call); REQUIRE(ticket->result()); lookup_call();
    auto lookup = std::get<LibraryBridge::Lookup>(result->value); REQUIRE(lookup.status == LibraryBridge::LookupStatus::hit);
    REQUIRE(lookup.value); REQUIRE(lookup.value->result.observed_through.epoch == 43); REQUIRE(delivered == 4);
    auto other = identity; other.configuration_id = "missing";
    bridge.lookup(audio, other, 44, [&](auto value){ result = std::move(value); ++delivered; });
    auto miss_call = queue.take(); REQUIRE(miss_call); miss_call(); REQUIRE(std::get<LibraryBridge::Lookup>(result->value).status == LibraryBridge::LookupStatus::miss);
    { std::ofstream out(audio, std::ios::binary | std::ios::app); out << 'x'; }
    bridge.lookup(audio, identity, 45, [&](auto value){ result = std::move(value); ++delivered; });
    auto changed_call = queue.take(); REQUIRE(changed_call); changed_call(); REQUIRE(std::get<LibraryBridge::Lookup>(result->value).status == LibraryBridge::LookupStatus::changed);
    REQUIRE(ticket->result()); REQUIRE(std::get<LibraryBridge::Lookup>(ticket->result()->value).status == LibraryBridge::LookupStatus::hit);
    bridge.entries({}, [&](auto){ ++delivered; }); auto fourth = queue.take(); REQUIRE(fourth); bridge.cancel(); fourth(); REQUIRE(delivered == 6);
    bridge.entries({}, [&](auto){ ++delivered; }); after_destroy = queue.take(); REQUIRE(after_destroy);
  }
  after_destroy(); REQUIRE(delivered == 6);
  std::error_code ignored; std::filesystem::remove_all(dir, ignored);
}

TEST_CASE("library bridge retains cancellation of pending requests") {
  auto dir = std::filesystem::temp_directory_path() / ("beatglass-cancel-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directory(dir);
  std::mutex mutex; std::condition_variable ready; bool entered = false, release = false;
  {
    LibraryBridge bridge((dir / "cache.sqlite").string(), [&](auto) {
      std::unique_lock lock(mutex); entered = true; ready.notify_one(); ready.wait(lock, [&] { return release; });
    });
    bridge.entries({}, {});
    { std::unique_lock lock(mutex); REQUIRE(ready.wait_for(lock, std::chrono::seconds(10), [&] { return entered; })); }
    auto pending = bridge.detail("track", {});
    bridge.cancel();
    auto result = pending->result(); REQUIRE(result); REQUIRE(result->cancelled);
    { std::lock_guard lock(mutex); release = true; } ready.notify_one();
  }
  std::error_code ignored; std::filesystem::remove_all(dir, ignored);
}
