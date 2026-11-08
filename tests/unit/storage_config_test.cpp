#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/storage_config.hpp"
#include "beatglass/library/storage_owner.hpp"
TEST_CASE("storage configuration requires an explicit database file") { std::string error; REQUIRE_FALSE(beatglass::library::valid_storage_config({}, &error)); REQUIRE_FALSE(error.empty()); REQUIRE(beatglass::library::valid_storage_config({"cache.sqlite"})); }
TEST_CASE("storage owner rejects invalid configuration before opening SQLite") { beatglass::library::StorageOwner owner; std::string error; REQUIRE_FALSE(owner.open({}, &error)); REQUIRE_FALSE(owner.cache()); REQUIRE_FALSE(error.empty()); }
TEST_CASE("storage owner clears a stale diagnostic before a successful open") { beatglass::library::StorageOwner owner; const auto path = std::filesystem::temp_directory_path() / "beatglass-storage-owner-diagnostic.sqlite"; std::error_code ec; std::filesystem::remove(path, ec); std::string error = "stale"; REQUIRE(owner.open({path}, &error)); CHECK(error.empty()); owner.close(); std::filesystem::remove(path, ec); }
TEST_CASE("storage owner exposes persisted retryable runs only while open") {
  beatglass::library::StorageOwner owner;
  CHECK(owner.retryable_runs().empty());
  const auto path = std::filesystem::temp_directory_path() / "beatglass-storage-owner-recovery.sqlite";
  std::error_code ec; std::filesystem::remove(path, ec);
  REQUIRE(owner.open({path}));
  CHECK(owner.retryable_runs().empty());
  owner.close(); CHECK(owner.retryable_runs().empty());
  std::filesystem::remove(path, ec);
}
TEST_CASE("storage owner discards retryable runs without touching source files") {
  beatglass::library::StorageOwner owner;
  const auto path = std::filesystem::temp_directory_path() / "beatglass-storage-owner-discard.sqlite";
  std::error_code ec; std::filesystem::remove(path, ec);
  REQUIRE(owner.open({path}));
  auto* cache = owner.cache(); REQUIRE(cache);
  const beatglass::library_detail::CacheKey key{"hash", "engine", "config", "offline"};
  cache->begin("retry", key);
  REQUIRE(cache->publish("retry", key, "discard", {}, [] { return false; }) == beatglass::library_detail::Publish::stale_input);
  REQUIRE(owner.retryable_runs().size() == 1);
  REQUIRE(owner.discard_retryable("retry"));
  CHECK(owner.retryable_runs().empty());
  CHECK_FALSE(owner.discard_retryable("retry"));
  owner.close(); std::filesystem::remove(path, ec);
}
#include <filesystem>
#include <system_error>
