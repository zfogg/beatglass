#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <chrono>
#include "../../lib/library/sqlite_cache.cpp"
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
    REQUIRE(cache.publish("interrupted", key, "partial", {}, [] { return true; }) == Publish::error);
    REQUIRE(cache.find(key, result)); REQUIRE(result == "stored");
  }
  REQUIRE(std::filesystem::remove(path));
}
TEST_CASE("SQLite cache observes cancellation immediately before commit") {
  SqliteCache cache(":memory:"); CacheKey key{"hash", "engine", "config", "offline"};
  cache.begin("one", key); std::stop_source stop;
  REQUIRE(cache.publish("one", key, "result", stop.get_token(), [&] { stop.request_stop(); return true; }) == Publish::cancelled);
  std::string result; REQUIRE_FALSE(cache.find(key, result));
}
