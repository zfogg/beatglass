#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include "beatglass/library/analysis_operation.hpp"
#include "beatglass/library/file_analysis_operation.hpp"
#include "beatglass/library/analysis_commit.hpp"
#include "beatglass/library/analysis_admission.hpp"
#include "beatglass/library/storage_owner.hpp"
TEST_CASE("analysis admission rejects incompatible decoder snapshots") {
  beatglass::library_detail::SqliteCache cache(":memory:");
  beatglass::library::AnalysisIdentity identity{"hash", "engine", "config", "offline"};
  const auto rejected = beatglass::library::admit_analysis(cache, "bad-snapshot", identity, "/music/a.flac", "file-reader-v0");
  CHECK_FALSE(rejected.decoder_compatible);
  CHECK_FALSE(rejected.analysis_reserved);
  CHECK_FALSE(rejected.cached.has_value());
}
TEST_CASE("analysis operation retains source identity and bounded callback contract") { beatglass::library::AnalysisOperation op; op.source.size_bytes=12; op.identity.mode="offline"; op.serialize=[] { return std::string("result"); }; REQUIRE(op.serialize()=="result"); REQUIRE(op.identity.mode=="offline"); }
TEST_CASE("analysis source identity includes a selected catalog path") {
  beatglass::library::AnalysisOperation op;
  op.source.path = "track.wav";
  REQUIRE_FALSE(op.source.path.empty());
}
TEST_CASE("analysis publication requires source and cache identities to match") {
  beatglass::library::AnalysisOperation op;
  op.source.content_hash = "sha256:source";
  op.identity.content_hash = "sha256:other";
  op.identity.engine_version = "engine";
  op.identity.configuration_id = "config";
  op.identity.mode = "offline";
  REQUIRE(op.source.content_hash != op.identity.content_hash);
}
TEST_CASE("decoder-only publication uses the stop-aware decoder and preserves run identity") {
    beatglass::library_detail::SqliteCache cache(":memory:");
    beatglass::library::AnalysisOperation op;
    op.source.path = "track.wav"; op.source.content_hash = "sha256:decode";
    op.identity = {"sha256:decode", "engine", "config", "offline"};
    op.decode = [](std::stop_token stop, const auto&) { return stop.stop_requested() ? std::string{} : std::string("decoded"); };
    op.verify = [](std::stop_token) { return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match}; };
    beatglass::library_detail::CacheKey key{"sha256:decode", "engine", "config", "offline"};
    cache.begin("decode-run", key);
    REQUIRE(beatglass::library::publish_operation(cache, "decode-run", op, {}) == beatglass::library_detail::Publish::committed);
    std::string result; REQUIRE(cache.find(key, result)); CHECK(result == "decoded");
}
TEST_CASE("storage owner exposes decoder publication boundary") {
    beatglass::library::StorageOwner owner;
    beatglass::library::AnalysisOperation operation;
    operation.source.path = "track.wav"; operation.source.content_hash = "sha256:closed";
    operation.identity = {"sha256:closed", "engine", "config", "offline"};
    operation.decode = [](std::stop_token, const auto&) { return std::string("decoded"); };
    operation.verify = [](std::stop_token) { return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match}; };
    CHECK(owner.publish_analysis("closed-run", operation) == beatglass::library_detail::Publish::error);
}
TEST_CASE("storage owner exposes cache admission boundary") {
    beatglass::library::StorageOwner owner;
    auto admission = owner.admit_analysis("closed-admission", {"sha256:closed", "engine", "config", "offline"});
    CHECK_FALSE(admission.cached.has_value()); CHECK_FALSE(admission.analysis_reserved);
}

TEST_CASE("coordinator admission replaces decoder on cache hit") {
  beatglass::library::AnalysisCoordinator coordinator;
  REQUIRE(coordinator.open_storage({":memory:"}));
  beatglass::AnalysisIdentity identity{"sha256:coordinator", "engine", "config", "offline"};
  beatglass::library_detail::CacheKey key{identity.content_hash, identity.engine_version, identity.configuration_id, identity.mode};
  coordinator.storage().cache()->begin("seed", key);
  REQUIRE(coordinator.storage().cache()->publish("seed", key, "cached", {}, [] { return true; }) == beatglass::library_detail::Publish::committed);
  beatglass::library::AnalysisOperation op;
  op.source.path = "track.wav"; op.source.content_hash = identity.content_hash;
  op.identity = identity; op.serialize = [] { return std::string("fallback"); };
  op.verify = [](std::stop_token) { return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match, {}}; };
  op.decode = [](std::stop_token, const auto&) { throw std::runtime_error("decoder must be skipped"); return std::string{}; };
  auto handle = coordinator.submit("cache-hit", std::move(op));
  REQUIRE(handle);
}
TEST_CASE("analysis cache reuse checks identity before running work") {
    beatglass::library_detail::SqliteCache cache(":memory:");
    beatglass::AnalysisIdentity identity{"sha256:cached", "engine", "config", "offline"};
    beatglass::library_detail::CacheKey key{identity.content_hash, identity.engine_version, identity.configuration_id, identity.mode};
    cache.begin("cached-run", key);
    REQUIRE(cache.publish("cached-run", key, "cached-result", {}, [] { return true; }) == beatglass::library_detail::Publish::committed);
    std::string result;
    REQUIRE(beatglass::library::reuse_cached_operation(cache, identity, result));
    CHECK(result == "cached-result");
    auto changed = identity; changed.configuration_id = "new-config";
    CHECK_FALSE(beatglass::library::reuse_cached_operation(cache, changed, result));
}
TEST_CASE("analysis admission reserves only cache misses") {
    beatglass::library_detail::SqliteCache cache(":memory:");
    beatglass::AnalysisIdentity identity{"sha256:admit", "engine", "config", "offline"};
    auto miss = beatglass::library::admit_analysis(cache, "admit-run", identity);
    CHECK_FALSE(miss.cached);
    REQUIRE(miss.analysis_reserved);
    beatglass::library_detail::CacheKey key{identity.content_hash, identity.engine_version, identity.configuration_id, identity.mode};
    REQUIRE(cache.publish("admit-run", key, "ready", {}, [] { return true; }) == beatglass::library_detail::Publish::committed);
    auto hit = beatglass::library::admit_analysis(cache, "unused", identity);
    REQUIRE(hit.cached);
    CHECK(*hit.cached == "ready");
    CHECK_FALSE(hit.analysis_reserved);
}
TEST_CASE("analysis publication rejects invalid operations before callbacks") {
  beatglass::library_detail::SqliteCache cache(":memory:");
  beatglass::library::AnalysisOperation op;
  op.source.path = "track.wav"; op.source.content_hash = "sha256:test";
  op.identity.content_hash = op.source.content_hash; op.identity.engine_version = "engine";
  op.identity.configuration_id.assign("config\0suffix", 13); op.identity.mode = "offline";
  bool serialized = false; bool verified = false;
  op.serialize = [&] { serialized = true; return std::string("result"); };
  op.verify = [&](std::stop_token) { verified = true; return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match}; };
  REQUIRE(beatglass::library::publish_operation(cache, "run", op, {}) == beatglass::library_detail::Publish::error);
  CHECK_FALSE(serialized); CHECK_FALSE(verified);
}
TEST_CASE("analysis publication observes cancellation before serialization") {
  beatglass::library_detail::SqliteCache cache(":memory:");
  beatglass::library::AnalysisOperation op;
  op.source.path = "track.wav"; op.source.content_hash = "sha256:test";
  op.identity.content_hash = op.source.content_hash; op.identity.engine_version = "engine";
  op.identity.configuration_id = "config"; op.identity.mode = "offline";
  bool serialized = false; bool verified = false;
  op.serialize = [&] { serialized = true; return std::string("result"); };
  op.verify = [&](std::stop_token) { verified = true; return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match}; };
  std::stop_source stop; stop.request_stop();
  REQUIRE(beatglass::library::publish_operation(cache, "run", op, stop.get_token()) == beatglass::library_detail::Publish::cancelled);
  CHECK_FALSE(serialized); CHECK_FALSE(verified);
}
TEST_CASE("analysis publication converts serialization failures into errors") {
  beatglass::library_detail::SqliteCache cache(":memory:");
  beatglass::library::AnalysisOperation op;
  op.source.path = "track.wav"; op.source.content_hash = "sha256:test";
  op.identity.content_hash = op.source.content_hash; op.identity.engine_version = "engine";
  op.identity.configuration_id = "config"; op.identity.mode = "offline";
  op.serialize = []() -> std::string { throw std::runtime_error("serialization failed"); };
  op.verify = [](std::stop_token) { return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match}; };
  cache.begin("run", {"sha256:test", "engine", "config", "offline"});
  REQUIRE(beatglass::library::publish_operation(cache, "run", op, {}) == beatglass::library_detail::Publish::error);
  REQUIRE(cache.retryable_runs().size() == 1);
  CHECK(cache.retryable_runs().front().id == "run");
}
TEST_CASE("analysis publication contains nonstandard serialization throws") {
  beatglass::library_detail::SqliteCache cache(":memory:");
  beatglass::library::AnalysisOperation op;
  op.source.path = "track.wav"; op.source.content_hash = "sha256:test";
  op.identity.content_hash = op.source.content_hash; op.identity.engine_version = "engine";
  op.identity.configuration_id = "config"; op.identity.mode = "offline";
  op.serialize = []() -> std::string { throw 7; };
  op.verify = [](std::stop_token) { return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match}; };
  REQUIRE(beatglass::library::publish_operation(cache, "run", op, {}) == beatglass::library_detail::Publish::error);
}
TEST_CASE("analysis publication stops after cancellation during serialization") {
  beatglass::library_detail::SqliteCache cache(":memory:");
  beatglass::library::AnalysisOperation op;
  op.source.path = "track.wav"; op.source.content_hash = "sha256:test";
  op.identity.content_hash = op.source.content_hash; op.identity.engine_version = "engine";
  op.identity.configuration_id = "config"; op.identity.mode = "offline";
  std::stop_source stop; bool verified = false;
  op.serialize = [&] { stop.request_stop(); return std::string("result"); };
  op.verify = [&](std::stop_token) { verified = true; return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match}; };
  REQUIRE(beatglass::library::publish_operation(cache, "run", op, stop.get_token()) == beatglass::library_detail::Publish::cancelled);
  CHECK_FALSE(verified);
}
TEST_CASE("file decoder retry accepts only the current snapshot") {
  CHECK(beatglass::library::file_decoder_snapshot_compatible(beatglass::library::kFileDecoderSnapshot));
  CHECK_FALSE(beatglass::library::file_decoder_snapshot_compatible(""));
  CHECK_FALSE(beatglass::library::file_decoder_snapshot_compatible("file-reader-v0"));
  CHECK_FALSE(beatglass::library::file_decoder_snapshot_compatible("decoder-v3;pcm-f32;44100"));
}
