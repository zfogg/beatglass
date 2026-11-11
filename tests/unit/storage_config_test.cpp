#include <fstream>
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
TEST_CASE("storage owner reopens the same database with retryable recovery intact") {
  beatglass::library::StorageOwner owner;
  const auto path = std::filesystem::temp_directory_path() / "beatglass-storage-owner-reopen-recovery.sqlite";
  std::error_code ec; std::filesystem::remove(path, ec);
  const beatglass::library_detail::CacheKey key{"reopen-hash", "engine", "config", "offline"};
  REQUIRE(owner.open({path}));
  owner.cache()->begin("reopen-run", key, "/music/reopen.wav");
  REQUIRE(owner.cache()->publish("reopen-run", key, "partial", {}, [] { return false; }) == beatglass::library_detail::Publish::stale_input);
  CHECK(owner.retryable_runs().size() == 1);
  owner.close();
  REQUIRE(owner.open({path}));
  const auto recovered = owner.retryable_runs();
  REQUIRE(recovered.size() == 1);
  CHECK(recovered.front().id == "reopen-run");
  CHECK(recovered.front().source_path == "/music/reopen.wav");
  owner.close(); std::filesystem::remove(path, ec);
}
TEST_CASE("storage owner closes the previous database before a failed reopen") {
  beatglass::library::StorageOwner owner;
  const auto path = std::filesystem::temp_directory_path() / "beatglass-storage-owner-failed-reopen.sqlite";
  std::error_code ec; std::filesystem::remove(path, ec);
  REQUIRE(owner.open({path}));
  REQUIRE(owner.cache());
  std::string diagnostic;
  CHECK_FALSE(owner.open({}, &diagnostic));
  CHECK_FALSE(owner.cache());
  CHECK(owner.retryable_runs().empty());
  CHECK_FALSE(diagnostic.empty());
  std::filesystem::remove(path, ec);
}
TEST_CASE("storage owner clears an old diagnostic after successful reopen") {
  beatglass::library::StorageOwner owner;
  const auto path = std::filesystem::temp_directory_path() / "beatglass-storage-owner-diagnostic.sqlite";
  std::error_code ec; std::filesystem::remove(path, ec);
  std::string diagnostic;
  CHECK_FALSE(owner.open({}, &diagnostic));
  REQUIRE_FALSE(diagnostic.empty());
  REQUIRE(owner.open({path}, &diagnostic));
  CHECK(diagnostic.empty());
  owner.close(); std::filesystem::remove(path, ec);
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
TEST_CASE("storage owner exposes atomic batch recovery discard") {
  beatglass::library::StorageOwner owner;
  const auto path = std::filesystem::temp_directory_path() / "beatglass-storage-owner-batch-discard.sqlite";
  std::error_code ec; std::filesystem::remove(path, ec);
  REQUIRE(owner.open({path}));
  const beatglass::library_detail::CacheKey key{"batch-hash", "engine", "config", "offline"};
  owner.cache()->begin("batch-one", key);
  REQUIRE(owner.cache()->publish("batch-one", key, "partial", {}, [] { return false; }) == beatglass::library_detail::Publish::stale_input);
  CHECK(owner.discard_retryable(std::vector<std::string>{"batch-one", "missing"}) == 1);
  CHECK(owner.retryable_runs().empty());
  owner.close(); std::filesystem::remove(path, ec);
}
TEST_CASE("storage owner exposes completed analysis cache reuse") {
  beatglass::library::StorageOwner owner;
  const auto path = std::filesystem::temp_directory_path() / "beatglass-storage-owner-cache.sqlite";
  std::error_code ec; std::filesystem::remove(path, ec);
  REQUIRE(owner.open({path}));
  const beatglass::AnalysisIdentity identity{"hash", "engine", "config", "offline"};
  const beatglass::library_detail::CacheKey key{"hash", "engine", "config", "offline"};
  owner.cache()->begin("run", key);
  REQUIRE(owner.cache()->publish("run", key, "cached", {}, [] { return true; }) == beatglass::library_detail::Publish::committed);
  std::string result; REQUIRE(owner.find_cached(identity, result)); CHECK(result == "cached");
  owner.close(); std::filesystem::remove(path, ec);
}

#if defined(BEATGLASS_LIBRARY_FINGERPRINT)
TEST_CASE("storage owner refreshes a persistent catalog entry from source evidence") {
  beatglass::library::StorageOwner owner;
  const auto database = std::filesystem::temp_directory_path() / "beatglass-storage-owner-catalog.sqlite";
  const auto source = std::filesystem::temp_directory_path() / "beatglass-storage-owner-catalog.wav";
  std::error_code ec;
  std::filesystem::remove(database, ec);
  std::filesystem::remove(source, ec);
  {
    std::ofstream output(source, std::ios::binary);
    output << "catalog evidence";
  }
  REQUIRE(owner.open({database}));
  std::string diagnostic;
  REQUIRE(owner.refresh_collection_file(source, &diagnostic));
  CHECK(diagnostic == "ok");
  const auto entry = owner.cache()->entry(source.string());
  REQUIRE(entry);
  CHECK(entry->name == source.filename().string());
  CHECK(entry->kind == "audio");
  CHECK(entry->size_bytes == 16);
  owner.close();
  std::filesystem::remove(database, ec);
  std::filesystem::remove(source, ec);
}
#endif
#include <filesystem>
#include <system_error>
