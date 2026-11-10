#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/detail/cache_identity.hpp"
#include <fstream>
#include <random>
using namespace beatglass::library_detail;
namespace {
struct Fixture {
 std::filesystem::path path = std::filesystem::temp_directory_path() / ("beatglass-cache-identity-" + std::to_string(std::random_device{}()) + ".wav");
 Fixture() { std::ofstream(path, std::ios::binary) << "identity"; }
 ~Fixture() { std::error_code ec; std::filesystem::remove(path, ec); }
};
}
TEST_CASE("cache identity derives a validated key from the observed file") {
 Fixture f; const auto result = cache_identity_from_file(f.path, "engine", "config", "offline");
 REQUIRE(result.status == FingerprintStatus::ok);
 CHECK(result.key.content.rfind("sha256:", 0) == 0);
 CHECK(result.key.engine == "engine");
 CHECK(result.key.config == "config");
 CHECK(result.key.mode == "offline");
}
TEST_CASE("cache identity never creates a key for missing or cancelled input") {
 const auto missing = cache_identity_from_file("missing-beatglass.wav", "e", "c", "offline");
 CHECK(missing.status == FingerprintStatus::error);
 std::stop_source stop; stop.request_stop(); Fixture f;
 const auto cancelled = cache_identity_from_file(f.path, "e", "c", "offline", stop.get_token());
 CHECK(cancelled.status == FingerprintStatus::cancelled);
 CHECK(cancelled.key.content.empty());
}

TEST_CASE("collection refresh publishes only a validated fingerprint") {
 Fixture f; SqliteCache cache(":memory:"); std::string diagnostic;
 REQUIRE(refresh_collection_file(cache, f.path, {}, &diagnostic) == FingerprintStatus::ok);
 const auto entry = cache.entry(f.path.string());
 REQUIRE(entry.has_value());
 CHECK(entry->fingerprint.rfind("sha256:", 0) == 0);
 CHECK(entry->size_bytes == 8);
 CHECK(entry->kind == "audio");
 std::stop_source stop; stop.request_stop();
 CHECK(refresh_collection_file(cache, f.path, stop.get_token(), &diagnostic) == FingerprintStatus::cancelled);
 CHECK(cache.entry(f.path.string()).has_value());
}

TEST_CASE("cache identity revalidation rejects changed content and cancellation") {
 Fixture f; const auto identity = cache_identity_from_file(f.path, "e", "c", "offline");
 REQUIRE(identity.status == FingerprintStatus::ok);
 CHECK(revalidate_file_identity(f.path, identity.key.content) == FingerprintStatus::ok);
 { std::ofstream out(f.path, std::ios::binary | std::ios::trunc); out << "changed"; }
 std::string diagnostic;
 CHECK(revalidate_file_identity(f.path, identity.key.content, {}, &diagnostic) == FingerprintStatus::changed);
 CHECK(diagnostic == "content identity changed");
 std::stop_source stop; stop.request_stop();
 CHECK(revalidate_file_identity(f.path, identity.key.content, stop.get_token()) == FingerprintStatus::cancelled);
}

TEST_CASE("stale identity invalidation removes only the active cached result") {
 Fixture f; SqliteCache cache(":memory:");
 const auto identity = cache_identity_from_file(f.path, "e", "c", "offline");
 REQUIRE(identity.status == FingerprintStatus::ok);
 cache.begin("run", identity.key);
 REQUIRE(cache.publish("run", identity.key, "cached", {}, [] { return true; }) == Publish::committed);
 CHECK(invalidate_if_stale(cache, identity.key, f.path) == FingerprintStatus::ok);
 std::string result; REQUIRE(cache.find(identity.key, result)); CHECK(result == "cached");
 { std::ofstream out(f.path, std::ios::binary | std::ios::trunc); out << "new bytes"; }
 CHECK(invalidate_if_stale(cache, identity.key, f.path) == FingerprintStatus::changed);
 CHECK_FALSE(cache.find(identity.key, result));
}

TEST_CASE("file publication revalidates before and at the commit boundary") {
 Fixture f; SqliteCache cache(":memory:");
 const auto identity = cache_identity_from_file(f.path, "e", "c", "offline");
 REQUIRE(identity.status == FingerprintStatus::ok);
 CHECK(publish_file_result(cache, "run", identity.key, f.path, "result") == Publish::committed);
 std::string result; REQUIRE(cache.find(identity.key, result)); CHECK(result == "result");
 { std::ofstream out(f.path, std::ios::binary | std::ios::trunc); out << "changed"; }
 CHECK(publish_file_result(cache, "retry", identity.key, f.path, "stale") == Publish::stale_input);
 CHECK(cache.find(identity.key, result)); CHECK(result == "result");
}

TEST_CASE("stale file publication can retry the same run after restoration") {
 Fixture f; SqliteCache cache(":memory:");
 const auto identity = cache_identity_from_file(f.path, "e", "c", "offline");
 REQUIRE(identity.status == FingerprintStatus::ok);
 REQUIRE(publish_file_result(cache, "run", identity.key, f.path, "first") == Publish::committed);
 { std::ofstream out(f.path, std::ios::binary | std::ios::trunc); out << "changed"; }
 CHECK(publish_file_result(cache, "retry", identity.key, f.path, "stale") == Publish::stale_input);
 std::string result; REQUIRE(cache.find(identity.key, result)); CHECK(result == "first");
 { std::ofstream out(f.path, std::ios::binary | std::ios::trunc); out << "identity"; }
 CHECK(publish_file_result(cache, "retry", identity.key, f.path, "second") == Publish::committed);
 REQUIRE(cache.find(identity.key, result)); CHECK(result == "second");
}

TEST_CASE("cancelled publication preserves the prior cached result") {
 Fixture f; SqliteCache cache(":memory:");
 const auto identity = cache_identity_from_file(f.path, "e", "c", "offline");
 REQUIRE(identity.status == FingerprintStatus::ok);
 REQUIRE(publish_file_result(cache, "run", identity.key, f.path, "first") == Publish::committed);
 std::stop_source stop; stop.request_stop();
 CHECK(publish_file_result(cache, "cancelled", identity.key, f.path, "second", stop.get_token()) == Publish::cancelled);
 std::string result; REQUIRE(cache.find(identity.key, result)); CHECK(result == "first");
}

TEST_CASE("stale catalog rows are removed only after failed identity validation") {
 Fixture f; SqliteCache cache(":memory:");
 const auto identity = cache_identity_from_file(f.path, "e", "c", "offline");
 REQUIRE(identity.status == FingerprintStatus::ok);
 REQUIRE(refresh_collection_file(cache, f.path) == FingerprintStatus::ok);
 CHECK(invalidate_collection_if_stale(cache, f.path.string(), identity.key.content) == FingerprintStatus::ok);
 REQUIRE(cache.entry(f.path.string()).has_value());
 { std::ofstream out(f.path, std::ios::binary | std::ios::trunc); out << "changed"; }
 CHECK(invalidate_collection_if_stale(cache, f.path.string(), identity.key.content) == FingerprintStatus::changed);
 CHECK_FALSE(cache.entry(f.path.string()).has_value());
}

TEST_CASE("missing catalog sources can be pruned without touching existing files") {
 Fixture f; SqliteCache cache(":memory:");
 REQUIRE(refresh_collection_file(cache, f.path) == FingerprintStatus::ok);
 CHECK_FALSE(prune_missing_collection_entry(cache, f.path));
 REQUIRE(cache.entry(f.path.string()).has_value());
 std::filesystem::remove(f.path);
 CHECK(prune_missing_collection_entry(cache, f.path));
 CHECK_FALSE(cache.entry(f.path.string()).has_value());
}
