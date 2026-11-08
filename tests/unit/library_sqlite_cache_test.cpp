#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <chrono>
#include <limits>
#include "beatglass/library/detail/sqlite_cache.hpp"
using namespace beatglass::library_detail;
TEST_CASE("SQLite cache replaces only completed revalidated runs") {
  SqliteCache cache(":memory:"); CacheKey key{"sha256:a", "engine1", "config1", "offline"};
  auto valid = [] { return true; }; std::string result;
  cache.begin("one", key);
  REQUIRE(cache.publish("one", key, "old", {}, valid) == Publish::committed);
  REQUIRE(cache.find(key, result)); REQUIRE(result == "old");
  cache.begin("two", key);
  REQUIRE(cache.publish("two", key, "new", {}, [] { return false; }) == Publish::stale_input);
  REQUIRE(cache.find(key, result)); REQUIRE(result == "old");
  std::stop_source cancelled; cancelled.request_stop();
  REQUIRE(cache.publish("two", key, "new", cancelled.get_token(), valid) == Publish::cancelled);
  REQUIRE(cache.find(key, result)); REQUIRE(result == "old");
  REQUIRE(cache.publish("two", key, "new", {}, valid) == Publish::committed);
  REQUIRE(cache.find(key, result)); REQUIRE(result == "new");
  auto changed = key; changed.config = "config2"; REQUIRE_FALSE(cache.find(changed, result));
  changed = key; changed.content = "sha256:b"; REQUIRE_FALSE(cache.find(changed, result));
}
TEST_CASE("SQLite cache rejects oversized results before they become active") {
  SqliteCache cache(":memory:"); CacheKey key{"hash", "engine", "config", "offline"};
  cache.begin("oversized", key);
  std::string result(kMaxCachedResultBytes + 1, 'x');
  REQUIRE(cache.publish("oversized", key, result, {}, [] { return true; }) == Publish::error);
  std::string found; REQUIRE_FALSE(cache.find(key, found));
}
TEST_CASE("SQLite cache rejects incomplete identities before creating runs") {
  SqliteCache cache(":memory:");
  REQUIRE_THROWS_AS(cache.begin("run", {"", "engine", "config", "offline"}), std::invalid_argument);
  REQUIRE_THROWS_AS(cache.begin("run", {"hash", "", "config", "offline"}), std::invalid_argument);
  REQUIRE_THROWS_AS(cache.begin("", {"hash", "engine", "config", "offline"}), std::invalid_argument);
}
TEST_CASE("SQLite cache lookup rejects malformed identities") {
  SqliteCache cache(":memory:");
  std::string result;
  REQUIRE_THROWS_AS(cache.find({"", "engine", "config", "offline"}, result), std::invalid_argument);
}
TEST_CASE("SQLite cache rolls back failed publication") {
  SqliteCache cache(":memory:"); CacheKey key{"hash", "engine", "config", "offline"};
  cache.begin("one", key);
  REQUIRE(cache.publish("one", key, "old", {}, [] { return true; }) == Publish::committed);
  REQUIRE(cache.publish("missing", key, "new", {}, [] { return true; }) == Publish::error);
  std::string result; REQUIRE(cache.find(key, result)); REQUIRE(result == "old");
}

TEST_CASE("SQLite cache persists completed work and recovers interrupted runs") {
  auto path = std::filesystem::temp_directory_path() /
    ("beatglass-cache-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".sqlite");
  CacheKey key{"hash", "engine", "config", "offline"};
  {
    SqliteCache cache(path.string());
    cache.begin("done", key);
    REQUIRE(cache.publish("done", key, "stored", {}, [] { return true; }) == Publish::committed);
    cache.begin("interrupted", key);
  }
  {
    SqliteCache cache(path.string()); std::string result;
    REQUIRE(cache.find(key, result)); REQUIRE(result == "stored");
    REQUIRE_NOTHROW(cache.begin("interrupted", key));
    REQUIRE(cache.publish("interrupted", key, "recovered", {}, [] { return true; }) == Publish::committed);
    REQUIRE(cache.find(key, result)); REQUIRE(result == "recovered");
  }
  REQUIRE(std::filesystem::remove(path));
}
TEST_CASE("SQLite cache observes cancellation immediately before commit") {
  SqliteCache cache(":memory:"); CacheKey key{"hash", "engine", "config", "offline"};
  cache.begin("one", key); std::stop_source stop;
  REQUIRE(cache.publish("one", key, "result", stop.get_token(), [&] { stop.request_stop(); return true; }) == Publish::cancelled);
  std::string result; REQUIRE_FALSE(cache.find(key, result));
}
TEST_CASE("SQLite binding lengths are checked before narrowing") {
  auto maximum = static_cast<std::size_t>(std::numeric_limits<int>::max());
  REQUIRE(sqlite_length_representable(maximum));
  REQUIRE_FALSE(sqlite_length_representable(maximum + 1));
}
TEST_CASE("SQLite cache exposes its on-disk schema version") {
  REQUIRE(kCacheSchemaVersion == 1);
}
TEST_CASE("SQLite cache holds exclusive ownership during recovery and use") {
  auto path = std::filesystem::temp_directory_path() /
    ("beatglass-exclusive-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".sqlite");
  {
    SqliteCache first(path.string());
    CacheKey key{"hash", "engine", "config", "offline"};
    first.begin("running", key);
    REQUIRE_THROWS(SqliteCache(path.string()));
    REQUIRE(first.publish("running", key, "result", {}, [] { return true; }) == Publish::committed);
  }
  { SqliteCache next(path.string()); std::string result; REQUIRE(next.find({"hash", "engine", "config", "offline"}, result)); }
  REQUIRE(std::filesystem::remove(path));
}
TEST_CASE("SQLite cache stores and pages collection metadata") {
  SqliteCache cache(":memory:");
  cache.put_entry({"/music/Beta.flac", "Beta.flac", "audio", "sha256:b", 20, 2});
  cache.put_entry({"/music/alpha.mp3", "alpha.mp3", "audio", "sha256:a", 10, 1});
  auto page = cache.entries({"A", 0, 10});
  REQUIRE(page.size() == 1); REQUIRE(page.front().path == "/music/alpha.mp3");
  REQUIRE(cache.entry("/music/Beta.flac")->size_bytes == 20);
  REQUIRE(cache.remove_entry("/music/Beta.flac")); REQUIRE_FALSE(cache.entry("/music/Beta.flac"));
}
TEST_CASE("SQLite cache invalidates one active identity without deleting its source") {
  SqliteCache cache(":memory:"); CacheKey key{"hash", "engine", "config", "offline"};
  cache.begin("run", key);
  REQUIRE(cache.publish("run", key, "result", {}, [] { return true; }) == Publish::committed);
  std::string result; REQUIRE(cache.find(key, result));
  REQUIRE(cache.invalidate(key)); REQUIRE_FALSE(cache.find(key, result));
  REQUIRE_FALSE(cache.invalidate(key));
}
