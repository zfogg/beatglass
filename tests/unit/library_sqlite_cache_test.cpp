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

TEST_CASE("SQLite binding lengths are checked before narrowing") {
  auto maximum = static_cast<std::size_t>(std::numeric_limits<int>::max());
  REQUIRE(sqlite_length_representable(maximum));
  REQUIRE_FALSE(sqlite_length_representable(maximum + 1));
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

TEST_CASE("reopen resolves committed and interrupted runs by exact ID and key") {
  auto path = std::filesystem::temp_directory_path() /
    ("beatglass-recovery-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".sqlite");
  CacheKey key{"hash", "engine", "config", "offline"};
  {
    SqliteCache cache(path.string());
    cache.begin("committed", key);
    REQUIRE(cache.publish("committed", key, "first", {}, [] { return true; }) == Publish::committed);
    cache.begin("later", key);
    REQUIRE(cache.publish("later", key, "second", {}, [] { return true; }) == Publish::committed);
    cache.begin("interrupted", key);
    REQUIRE_THROWS(cache.recover("interrupted", key));
  }
  {
    SqliteCache cache(path.string());
    REQUIRE(cache.recover("committed", key) == RunRecovery::committed);
    REQUIRE(cache.recover("interrupted", key) == RunRecovery::retryable);
    REQUIRE(cache.recover("missing", key) == RunRecovery::missing);
    auto wrong = key; wrong.config = "other";
    REQUIRE(cache.recover("committed", wrong) == RunRecovery::missing);
    std::string result; REQUIRE(cache.find(key, result)); REQUIRE(result == "second");
  }
  REQUIRE(std::filesystem::remove(path));
}

TEST_CASE("lost commit acknowledgement is resolved from persisted run ID") {
  auto path = std::filesystem::temp_directory_path() /
    ("beatglass-commit-fault-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".sqlite");
  CacheKey key{"hash", "engine", "config", "offline"};
  {
    SqliteCache cache(path.string()); cache.begin("run", key);
    REQUIRE(cache.publish("run", key, "result", {}, [] { return true; }, [] { throw std::runtime_error("lost acknowledgement"); }) == Publish::error);
  }
  {
    SqliteCache cache(path.string());
    REQUIRE(cache.recover("run", key) == RunRecovery::committed);
    std::string result; REQUIRE(cache.find(key, result)); REQUIRE(result == "result");
  }
  REQUIRE(std::filesystem::remove(path));
}
TEST_CASE("failure before commit preserves old result and interrupted run is retryable") {
  auto path = std::filesystem::temp_directory_path() /
    ("beatglass-rollback-fault-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".sqlite");
  CacheKey key{"hash", "engine", "config", "offline"};
  {
    SqliteCache cache(path.string()); cache.begin("old", key);
    REQUIRE(cache.publish("old", key, "old", {}, [] { return true; }) == Publish::committed);
    cache.begin("new", key);
    REQUIRE(cache.publish("new", key, "new", {}, []() -> bool { throw std::runtime_error("verification failed"); }) == Publish::error);
  }
  {
    SqliteCache cache(path.string());
    REQUIRE(cache.recover("new", key) == RunRecovery::retryable);
    std::string result; REQUIRE(cache.find(key, result)); REQUIRE(result == "old");
  }
  REQUIRE(std::filesystem::remove(path));
}

TEST_CASE("cancellation after commit starts cannot relabel a committed run") {
  SqliteCache cache(":memory:"); CacheKey key{"hash", "engine", "config", "offline"}; std::stop_source stop;
  cache.begin("run", key);
  auto result = cache.publish("run", key, "completed", stop.get_token(), [] { return true; }, [&] { stop.request_stop(); });
  REQUIRE(stop.stop_requested()); REQUIRE(result == Publish::committed);
  std::string value; REQUIRE(cache.find(key, value)); REQUIRE(value == "completed");
}
