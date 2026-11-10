#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <chrono>
#include <limits>
#include <stdexcept>
#include "beatglass/library/detail/sqlite_cache.hpp"
#include "beatglass/library/file_analysis_operation.hpp"
using namespace beatglass::library_detail;
namespace {
std::string embedded_nul_path() {
  return std::string("/music/a") + '\0' + ".flac";
}
std::string embedded_nul_search() {
  return std::string("a") + '\0';
}
}
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
TEST_CASE("SQLite cache persists an empty analysis result distinctly from a miss") {
  SqliteCache cache(":memory:"); CacheKey key{"empty", "engine", "config", "offline"};
  cache.begin("empty-run", key);
  REQUIRE(cache.publish("empty-run", key, "", {}, [] { return true; }) == Publish::committed);
  std::string result("sentinel"); REQUIRE(cache.find(key, result)); REQUIRE(result.empty());
  CacheKey missing = key; missing.content = "missing";
  REQUIRE_FALSE(cache.find(missing, result));
}
TEST_CASE("SQLite cache acceptance reuses unchanged identity without rerunning analysis") {
  SqliteCache cache(":memory:");
  CacheKey key{"sha256:unchanged", "engine", "config", "offline"};
  int analysis_runs = 0;
  auto analyze_if_missing = [&] {
    std::string cached;
    if (cache.find(key, cached)) return cached;
    ++analysis_runs;
    cache.begin("unchanged-run", key);
    REQUIRE(cache.publish("unchanged-run", key, "result", {}, [] { return true; }) == Publish::committed);
    return std::string("result");
  };
  REQUIRE(analyze_if_missing() == "result");
  REQUIRE(analyze_if_missing() == "result");
  CHECK(analysis_runs == 1);
}
TEST_CASE("SQLite cache acceptance reruns changed content and configuration identities") {
  SqliteCache cache(":memory:");
  const CacheKey original{"sha256:original", "engine", "config-v1", "offline"};
  int analysis_runs = 0;
  auto analyze_if_missing = [&](const CacheKey& key, const std::string& run_id) {
    std::string cached;
    if (cache.find(key, cached)) return cached;
    ++analysis_runs;
    cache.begin(run_id, key);
    REQUIRE(cache.publish(run_id, key, "result-" + std::to_string(analysis_runs), {}, [] { return true; }) == Publish::committed);
    return std::string("result-") + std::to_string(analysis_runs);
  };
  REQUIRE(analyze_if_missing(original, "original") == "result-1");
  auto changed_content = original; changed_content.content = "sha256:changed";
  REQUIRE(analyze_if_missing(changed_content, "changed-content") == "result-2");
  auto changed_config = original; changed_config.config = "config-v2";
  REQUIRE(analyze_if_missing(changed_config, "changed-config") == "result-3");
  CHECK(analysis_runs == 3);
}
TEST_CASE("SQLite cache rejects incomplete identities before creating runs") {
  SqliteCache cache(":memory:");
  REQUIRE_THROWS_AS(cache.begin("run", {"", "engine", "config", "offline"}), std::invalid_argument);
  REQUIRE_THROWS_AS(cache.begin("run", {"hash", "", "config", "offline"}), std::invalid_argument);
  REQUIRE_THROWS_AS(cache.begin("", {"hash", "engine", "config", "offline"}), std::invalid_argument);
}
TEST_CASE("SQLite cache bounds persisted retry snapshots") {
  SqliteCache cache(":memory:");
  const CacheKey key{"hash", "engine", "config", "offline"};
  REQUIRE_THROWS_AS(cache.begin("run", key, std::string(kMaxCollectionPathBytes + 1, 'x')), std::invalid_argument);
  REQUIRE_THROWS_AS(cache.begin("run", key, "/music/a.flac", std::string(kMaxCacheConfigurationBytes + 1, 'x')), std::invalid_argument);
  REQUIRE_THROWS_AS(cache.begin("run", key, std::string("/music/\0x", 9)), std::invalid_argument);
  REQUIRE_THROWS_AS(cache.begin("run", key, "/music/a.flac", std::string("decoder\0v3", 10)), std::invalid_argument);
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
TEST_CASE("SQLite cache rolls back when revalidation throws") {
  SqliteCache cache(":memory:"); CacheKey key{"hash", "engine", "config", "offline"};
  cache.begin("old", key);
  REQUIRE(cache.publish("old", key, "stable", {}, [] { return true; }) == Publish::committed);
  cache.begin("new", key);
  REQUIRE(cache.publish("new", key, "discard", {}, []() -> bool { throw std::runtime_error("verification failed"); }) == Publish::error);
  std::string result; REQUIRE(cache.find(key, result)); REQUIRE(result == "stable");
  const auto retryable = cache.retryable_runs();
  REQUIRE(retryable.size() == 1);
  CHECK(retryable.front().id == "new");
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
  const auto retryable = cache.retryable_runs(); REQUIRE(retryable.size() == 1); CHECK(retryable.front().id == "interrupted"); CHECK(retryable.front().key.content == key.content);
    REQUIRE_NOTHROW(cache.begin("interrupted", key));
    REQUIRE(cache.publish("interrupted", key, "recovered", {}, [] { return true; }) == Publish::committed);
    REQUIRE(cache.find(key, result)); REQUIRE(result == "recovered");
  }
  REQUIRE(std::filesystem::remove(path));
}

TEST_CASE("SQLite cache reuses durable identity and reruns changed content or configuration") {
  const auto path = std::filesystem::temp_directory_path() /
    ("beatglass-cache-acceptance-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".sqlite");
  const CacheKey unchanged{"sha256:file-v1", "engine-v1", "config-a", "offline"};
  {
    SqliteCache cache(path.string());
    cache.begin("initial", unchanged, "/music/acceptance.wav", beatglass::library::kFileDecoderSnapshot);
    REQUIRE(cache.publish("initial", unchanged, "analysis-v1", {}, [] { return true; }) == Publish::committed);
  }
  {
    SqliteCache cache(path.string());
    std::string result;
    CHECK(cache.find(unchanged, result));
    CHECK(result == "analysis-v1");

    const CacheKey changed_content{"sha256:file-v2", "engine-v1", "config-a", "offline"};
    CHECK_FALSE(cache.find(changed_content, result));
    cache.begin("content-change", changed_content, "/music/acceptance.wav", beatglass::library::kFileDecoderSnapshot);
    REQUIRE(cache.publish("content-change", changed_content, "analysis-v2", {}, [] { return true; }) == Publish::committed);

    const CacheKey changed_configuration{"sha256:file-v1", "engine-v1", "config-b", "offline"};
    CHECK_FALSE(cache.find(changed_configuration, result));
    cache.begin("config-change", changed_configuration, "/music/acceptance.wav", beatglass::library::kFileDecoderSnapshot);
    REQUIRE(cache.publish("config-change", changed_configuration, "analysis-config-b", {}, [] { return true; }) == Publish::committed);
  }
  REQUIRE(std::filesystem::remove(path));
}

TEST_CASE("SQLite cache observes cancellation immediately before commit") {
  SqliteCache cache(":memory:"); CacheKey key{"hash", "engine", "config", "offline"};
  cache.begin("one", key); std::stop_source stop;
  REQUIRE(cache.publish("one", key, "result", stop.get_token(), [&] { stop.request_stop(); return true; }) == Publish::cancelled);
  std::string result; REQUIRE_FALSE(cache.find(key, result));
  // Cancellation must leave the run retryable rather than permanently
  // running, so a replacement operation can begin without a restart.
  REQUIRE_NOTHROW(cache.begin("one", key));
  REQUIRE(cache.publish("one", key, "retry", {}, [] { return true; }) == Publish::committed);
  REQUIRE(cache.find(key, result)); REQUIRE(result == "retry");
}

TEST_CASE("SQLite cache makes stale input runs immediately retryable") {
  SqliteCache cache(":memory:"); CacheKey key{"hash", "engine", "config", "offline"};
  cache.begin("stale", key);
  REQUIRE(cache.publish("stale", key, "discard", {}, [] { return false; }) == Publish::stale_input);
  REQUIRE_NOTHROW(cache.begin("stale", key));
  std::string result;
  REQUIRE(cache.publish("stale", key, "fresh", {}, [] { return true; }) == Publish::committed);
  REQUIRE(cache.find(key, result)); REQUIRE(result == "fresh");
}
TEST_CASE("SQLite cache discards only retryable runs") {
  SqliteCache cache(":memory:"); CacheKey key{"hash", "engine", "config", "offline"};
  cache.begin("retry", key);
  REQUIRE(cache.discard_retryable("retry") == false);
  REQUIRE(cache.publish("retry", key, "discard", {}, [] { return false; }) == Publish::stale_input);
  REQUIRE(cache.discard_retryable("retry"));
  REQUIRE_FALSE(cache.discard_retryable("retry"));
  REQUIRE_THROWS_AS(cache.discard_retryable(""), std::invalid_argument);
}
TEST_CASE("SQLite cache preserves retryable rows when batch validation fails") {
  SqliteCache cache(":memory:"); CacheKey key{"hash", "engine", "config", "offline"};
  cache.begin("keep", key);
  REQUIRE(cache.publish("keep", key, "partial", {}, [] { return false; }) == Publish::stale_input);
  REQUIRE_THROWS_AS(cache.discard_retryable(std::vector<std::string>{"keep", std::string("bad\0id", 6)}), std::invalid_argument);
  REQUIRE(cache.retryable_runs().size() == 1);
  CHECK(cache.retryable_runs().front().id == "keep");
}
TEST_CASE("SQLite cache rolls back a batch discard when a later ID is malformed") {
  SqliteCache cache(":memory:"); CacheKey key{"hash", "engine", "config", "offline"};
  cache.begin("keep", key);
  REQUIRE(cache.publish("keep", key, "partial", {}, [] { return false; }) == Publish::stale_input);
  REQUIRE_THROWS_AS(cache.discard_retryable(std::vector<std::string>{"keep", std::string("bad\0id", 6)}), std::invalid_argument);
  REQUIRE(cache.retryable_runs().size() == 1);
  CHECK(cache.retryable_runs().front().id == "keep");
}
TEST_CASE("SQLite cache rejects duplicate batch discard IDs before deleting") {
  SqliteCache cache(":memory:"); CacheKey key{"hash", "engine", "config", "offline"};
  cache.begin("keep", key);
  REQUIRE(cache.publish("keep", key, "partial", {}, [] { return false; }) == Publish::stale_input);
  REQUIRE_THROWS_AS(cache.discard_retryable(std::vector<std::string>{"keep", "keep"}), std::invalid_argument);
  CHECK(cache.retryable_runs().size() == 1);
}
TEST_CASE("SQLite cache discards retryable runs atomically as a batch") {
  SqliteCache cache(":memory:"); CacheKey key{"hash", "engine", "config", "offline"};
  cache.begin("one", key); REQUIRE(cache.publish("one", key, "x", {}, [] { return false; }) == Publish::stale_input);
  cache.begin("two", key); REQUIRE(cache.publish("two", key, "x", {}, [] { return false; }) == Publish::stale_input);
  CHECK(cache.discard_retryable(std::vector<std::string>{"one", "two", "missing"}) == 2);
  CHECK(cache.retryable_runs().empty());
  REQUIRE_THROWS_AS(cache.discard_retryable(std::vector<std::string>{"ok", std::string("bad\0id", 6)}), std::invalid_argument);
}
TEST_CASE("SQLite retryable run query exposes its bound") {
  SqliteCache cache(":memory:");
  CHECK(kMaxRetryableRuns == 10000);
  CHECK_THROWS_AS(cache.retryable_runs(kMaxRetryableRuns + 1), std::invalid_argument);
}
TEST_CASE("SQLite retryable runs preserve source and decoder snapshots") {
  SqliteCache cache(":memory:");
  CacheKey key{"hash", "engine", "config", "offline"};
  cache.begin("snapshot", key, "/music/example.flac", "decoder-v3;pcm-f32;44100");
  REQUIRE(cache.publish("snapshot", key, "discard", {}, [] { return false; }) == Publish::stale_input);
  const auto runs = cache.retryable_runs();
  REQUIRE(runs.size() == 1);
  CHECK(runs.front().source_path == "/music/example.flac");
  CHECK(runs.front().decoder_snapshot == "decoder-v3;pcm-f32;44100");
}
TEST_CASE("SQLite legacy retryable runs preserve an empty decoder snapshot") {
  SqliteCache cache(":memory:");
  CacheKey key{"legacy-hash", "engine", "config", "offline"};
  cache.begin("legacy-snapshot", key, "/music/legacy.flac");
  REQUIRE(cache.publish("legacy-snapshot", key, "discard", {}, [] { return false; }) == Publish::stale_input);
  const auto runs = cache.retryable_runs();
  REQUIRE(runs.size() == 1);
  CHECK(runs.front().source_path == "/music/legacy.flac");
  CHECK(runs.front().decoder_snapshot.empty());
}
TEST_CASE("SQLite cache run IDs expose their API bound") {
  SqliteCache cache(":memory:");
  CHECK(kMaxCacheRunIdBytes == 4096);
  CacheKey key{"hash", "engine", "config", "offline"};
  CHECK_THROWS_AS(cache.begin(std::string(kMaxCacheRunIdBytes + 1, 'x'), key), std::invalid_argument);
}
TEST_CASE("SQLite cache content identities expose their API bound") {
  SqliteCache cache(":memory:");
  CHECK(kMaxCacheContentIdentityBytes == 512);
  CacheKey key{std::string(kMaxCacheContentIdentityBytes + 1, 'x'), "engine", "config", "offline"};
  CHECK_THROWS_AS(cache.begin("run", key), std::invalid_argument);
}
TEST_CASE("SQLite cache engine versions expose their API bound") {
  SqliteCache cache(":memory:");
  CHECK(kMaxCacheEngineVersionBytes == 4096);
  CacheKey key{"hash", std::string(kMaxCacheEngineVersionBytes + 1, 'x'), "config", "offline"};
  CHECK_THROWS_AS(cache.begin("run", key), std::invalid_argument);
}
TEST_CASE("SQLite cache configurations expose their API bound") {
  SqliteCache cache(":memory:");
  CHECK(kMaxCacheConfigurationBytes == 4096);
  CacheKey key{"hash", "engine", std::string(kMaxCacheConfigurationBytes + 1, 'x'), "offline"};
  CHECK_THROWS_AS(cache.begin("run", key), std::invalid_argument);
}
TEST_CASE("SQLite cache modes expose their API bound") {
  SqliteCache cache(":memory:");
  CHECK(kMaxCacheModeBytes == 128);
  CacheKey key{"hash", "engine", "config", std::string(kMaxCacheModeBytes + 1, 'x')};
  CHECK_THROWS_AS(cache.begin("run", key), std::invalid_argument);
}
TEST_CASE("SQLite binding lengths are checked before narrowing") {
  auto maximum = static_cast<std::size_t>(std::numeric_limits<int>::max());
  REQUIRE(sqlite_length_representable(maximum));
  REQUIRE_FALSE(sqlite_length_representable(maximum + 1));
}
TEST_CASE("SQLite cache exposes its on-disk schema version") {
  REQUIRE(kCacheSchemaVersion == 2);
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
TEST_CASE("SQLite collection reads expose typed cancellation") {
  SqliteCache cache(":memory:");
  cache.put_entry({"/music/a.flac", "a.flac", "audio", "hash", 1, 1});
  std::stop_source stop;
  stop.request_stop();
  CHECK_THROWS_AS(cache.entry("/music/a.flac", stop.get_token()), CollectionCancelled);
  CHECK_THROWS_AS(cache.entries({"", 0, 10}, stop.get_token()), CollectionCancelled);
}
TEST_CASE("SQLite exact catalog lookup rejects unsafe paths") {
  SqliteCache cache(":memory:");
  CHECK_THROWS_AS(cache.entry(""), std::invalid_argument);
  CHECK_THROWS_AS(cache.entry(embedded_nul_path()), std::invalid_argument);
}
TEST_CASE("SQLite catalog prefix queries bound search input") {
  SqliteCache cache(":memory:");
  CHECK(kMaxCollectionSearchBytes == 1024);
  CHECK_THROWS_AS(cache.entries({std::string(kMaxCollectionSearchBytes + 1, 'a'), 0, 10}), std::invalid_argument);
}
TEST_CASE("SQLite collection page size exposes its API bound") {
  SqliteCache cache(":memory:");
  CHECK(kMaxCollectionPageSize == 10000);
  CHECK_THROWS_AS(cache.entries({"", 0, kMaxCollectionPageSize + 1}), std::invalid_argument);
}
TEST_CASE("SQLite collection offset exposes its signed binding bound") {
  SqliteCache cache(":memory:");
  CHECK(kMaxCollectionOffset == static_cast<std::size_t>(0x7fffffffffffffffULL));
}
TEST_CASE("SQLite collection sizes are checked before signed binding") {
  SqliteCache cache(":memory:");
  CHECK_THROWS_AS(cache.put_entry({"/music/a.flac", "a.flac", "audio", "hash",
    std::numeric_limits<std::uint64_t>::max(), 1}), std::invalid_argument);
}
TEST_CASE("SQLite collection fingerprints expose their API bound") {
  SqliteCache cache(":memory:");
  CHECK(kMaxCollectionFingerprintBytes == 4096);
  CHECK_THROWS_AS(cache.put_entry({"/music/a.flac", "a.flac", "audio",
    std::string(kMaxCollectionFingerprintBytes + 1, 'x'), 1, 1}), std::invalid_argument);
}
TEST_CASE("SQLite collection names expose their API bound") {
  SqliteCache cache(":memory:");
  CHECK(kMaxCollectionNameBytes == 1024);
  CHECK_THROWS_AS(cache.put_entry({"/music/a.flac", std::string(kMaxCollectionNameBytes + 1, 'x'),
    "audio", "hash", 1, 1}), std::invalid_argument);
}
TEST_CASE("SQLite collection kinds expose their API bound") {
  SqliteCache cache(":memory:");
  CHECK(kMaxCollectionKindBytes == 128);
  CHECK_THROWS_AS(cache.put_entry({"/music/a.flac", "a.flac",
    std::string(kMaxCollectionKindBytes + 1, 'x'), "hash", 1, 1}), std::invalid_argument);
}
TEST_CASE("SQLite collection paths expose their API bound") {
  SqliteCache cache(":memory:");
  CHECK(kMaxCollectionPathBytes == 4096);
  CHECK_THROWS_AS(cache.put_entry({std::string(kMaxCollectionPathBytes + 1, 'x'),
    "a.flac", "audio", "hash", 1, 1}), std::invalid_argument);
}
TEST_CASE("SQLite collection removal uses the path bound") {
  SqliteCache cache(":memory:");
  CHECK_THROWS_AS(cache.remove_entry(std::string(kMaxCollectionPathBytes + 1, 'x')), std::invalid_argument);
}
TEST_CASE("SQLite collection metadata rejects unsafe text before SQLite binding") {
  SqliteCache cache(":memory:");
  CHECK_THROWS_AS(cache.put_entry({"", "name", "audio", "hash", 1, 1}), std::invalid_argument);
  CHECK_THROWS_AS(cache.put_entry({embedded_nul_path(), "a.flac", "audio", "hash", 1, 1}), std::invalid_argument);
  CHECK_THROWS_AS(cache.put_entry({"/music/a.flac", "a.flac", "audio", "", 1, 1}), std::invalid_argument);
  CHECK_THROWS_AS(cache.remove_entry(embedded_nul_path()), std::invalid_argument);
  CHECK_THROWS_AS(cache.entries({embedded_nul_search(), 0, 10}), std::invalid_argument);
}
TEST_CASE("SQLite cache invalidates one active identity without deleting its source") {
  SqliteCache cache(":memory:"); CacheKey key{"hash", "engine", "config", "offline"};
  cache.begin("run", key);
  REQUIRE(cache.publish("run", key, "result", {}, [] { return true; }) == Publish::committed);
  std::string result; REQUIRE(cache.find(key, result));
  REQUIRE(cache.invalidate(key)); REQUIRE_FALSE(cache.find(key, result));
  REQUIRE_FALSE(cache.invalidate(key));
}
