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
