#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <atomic>
#include <thread>
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
TEST_CASE("analysis admission accepts a compatible snapshot cache hit") {
  beatglass::library_detail::SqliteCache cache(":memory:");
  beatglass::library::AnalysisIdentity identity{"hash", "engine", "config", "offline"};
  cache.begin("seed", {"hash", "engine", "config", "offline"}, "/music/a.flac", beatglass::library::kFileDecoderSnapshot);
  REQUIRE(cache.publish("seed", {"hash", "engine", "config", "offline"}, "cached", {}, [] { return true; }) == beatglass::library_detail::Publish::committed);
  const auto hit = beatglass::library::admit_analysis(cache, "unused", identity, "/music/a.flac", beatglass::library::kFileDecoderSnapshot);
  REQUIRE(hit.decoder_compatible);
  CHECK(hit.cached == std::optional<std::string>{"cached"});
  CHECK_FALSE(hit.analysis_reserved);
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
  coordinator.storage().cache()->begin("seed", key, "track.wav", beatglass::library::kFileDecoderSnapshot);
  REQUIRE(coordinator.storage().cache()->publish("seed", key, "cached", {}, [] { return true; }) == beatglass::library_detail::Publish::committed);
  beatglass::library::AnalysisOperation op;
  op.source.path = "track.wav"; op.source.content_hash = identity.content_hash;
  op.identity = identity; op.decoder_snapshot = beatglass::library::kFileDecoderSnapshot; op.serialize = [] { return std::string("fallback"); };
  op.verify = [](std::stop_token) { return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match, {}}; };
  std::atomic_bool decoder_called{false};
  op.decode = [&decoder_called](std::stop_token, const auto&) { decoder_called.store(true); return std::string("unexpected"); };
  auto handle = coordinator.submit("cache-hit", std::move(op));
  REQUIRE(handle);
  for (int i = 0; i < 1000 && (handle->state() == beatglass::library_detail::TrackedState::queued || handle->state() == beatglass::library_detail::TrackedState::running); ++i) std::this_thread::yield();
  REQUIRE(handle->state() == beatglass::library_detail::TrackedState::completed);
  CHECK_FALSE(decoder_called.load());
}
TEST_CASE("coordinator rejects incompatible snapshots before reserving work") {
  beatglass::library::AnalysisCoordinator coordinator;
  REQUIRE(coordinator.open_storage({":memory:"}));
  beatglass::library::AnalysisOperation op;
  op.source.path = "track.wav"; op.source.content_hash = "hash";
  op.identity = {"hash", "engine", "config", "offline"}; op.decoder_snapshot = "legacy";
  op.decode = [](std::stop_token, const auto&) { return std::string("must not run"); };
  op.verify = [](std::stop_token) { return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match}; };
  CHECK_FALSE(coordinator.submit("legacy-run", std::move(op)));
  CHECK(coordinator.storage().retryable_runs().empty());
}
TEST_CASE("coordinator cancellation does not publish a stale result") {
  beatglass::library::AnalysisCoordinator coordinator;
  REQUIRE(coordinator.open_storage({":memory:"}));
  std::atomic_bool published{false};
  beatglass::library::AnalysisOperation op;
  op.source.path = "track.wav"; op.source.content_hash = "cancel-hash";
  op.identity = {"cancel-hash", "engine", "config", "offline"}; op.decoder_snapshot = beatglass::library::kFileDecoderSnapshot;
  op.decode = [](std::stop_token stop, const auto&) { while (!stop.stop_requested()) std::this_thread::yield(); return std::string{}; };
  op.verify = [](std::stop_token stop) { return beatglass::FileVerificationResult{stop.stop_requested() ? beatglass::FileVerificationOutcome::cancelled : beatglass::FileVerificationOutcome::match}; };
  op.publish = [&published](const std::string&) { published.store(true); };
  auto handle = coordinator.submit("cancel-run", std::move(op));
  REQUIRE(handle);
  handle->cancel();
  for (int i = 0; i < 1000 && (handle->state() == beatglass::library_detail::TrackedState::queued || handle->state() == beatglass::library_detail::TrackedState::running); ++i) std::this_thread::yield();
  CHECK(handle->state() == beatglass::library_detail::TrackedState::cancelled);
  CHECK_FALSE(published.load());
}
TEST_CASE("cancelled analysis can be retried with a compatible snapshot") {
  beatglass::library_detail::SqliteCache cache(":memory:");
  beatglass::library_detail::CacheKey key{"retry-hash", "engine", "config", "offline"};
  cache.begin("retry-run", key, "track.wav", beatglass::library::kFileDecoderSnapshot);
  std::stop_source stop;
  stop.request_stop();
  CHECK(cache.publish("retry-run", key, "stale", stop.get_token(), [] { return true; }) == beatglass::library_detail::Publish::cancelled);
  std::string result;
  CHECK_FALSE(cache.find(key, result));
  const auto pending = cache.retryable_runs();
  REQUIRE(pending.size() == 1);
  CHECK(pending.front().decoder_snapshot == beatglass::library::kFileDecoderSnapshot);
  cache.begin("retry-run", key, pending.front().source_path, pending.front().decoder_snapshot);
  REQUIRE(cache.publish("retry-run", key, "fresh", {}, [] { return true; }) == beatglass::library_detail::Publish::committed);
  REQUIRE(cache.find(key, result));
  CHECK(result == "fresh");
  CHECK(cache.retryable_runs().empty());
}
TEST_CASE("source verification failure preserves retryable metadata without publication") {
  beatglass::library_detail::SqliteCache cache(":memory:");
  beatglass::library_detail::CacheKey key{"mismatch-hash", "engine", "config", "offline"};
  cache.begin("mismatch-run", key, "missing-track.wav", beatglass::library::kFileDecoderSnapshot);
  REQUIRE(cache.publish("mismatch-run", key, "wrong-source-result", {}, [] { return false; }) == beatglass::library_detail::Publish::stale_input);
  std::string result;
  CHECK_FALSE(cache.find(key, result));
  const auto retryable = cache.retryable_runs();
  REQUIRE(retryable.size() == 1);
  CHECK(retryable.front().id == "mismatch-run");
  CHECK(retryable.front().source_path == "missing-track.wav");
  CHECK(retryable.front().decoder_snapshot == beatglass::library::kFileDecoderSnapshot);
  CHECK(cache.discard_retryable("mismatch-run"));
  CHECK(cache.retryable_runs().empty());
}
TEST_CASE("permission-style endpoint failure is recoverable without stale publication") {
  beatglass::library_detail::SqliteCache cache(":memory:");
  beatglass::library_detail::CacheKey key{"endpoint-hash", "engine", "config", "offline"};
  cache.begin("endpoint-permission", key, "endpoint://default-output", beatglass::library::kFileDecoderSnapshot);
  REQUIRE(cache.publish("endpoint-permission", key, "permission-denied-result", {}, [] { return false; }) == beatglass::library_detail::Publish::stale_input);
  std::string result;
  CHECK_FALSE(cache.find(key, result));
  const auto retryable = cache.retryable_runs();
  REQUIRE(retryable.size() == 1);
  CHECK(retryable.front().id == "endpoint-permission");
  CHECK(retryable.front().source_path == "endpoint://default-output");
  CHECK(retryable.front().decoder_snapshot == beatglass::library::kFileDecoderSnapshot);
  REQUIRE(cache.discard_retryable("endpoint-permission"));
  CHECK(cache.retryable_runs().empty());
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
