#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <atomic>
#include <thread>
#include <filesystem>
#include <chrono>
#include <fstream>
#include "beatglass/library/analysis_operation.hpp"
#include "beatglass/library/file_analysis_operation.hpp"
#include "beatglass/library/analysis_commit.hpp"
#include "beatglass/library/analysis_admission.hpp"
#include "beatglass/library/storage_owner.hpp"
#include "beatglass/library/analysis_coordinator.hpp"
#include "beatglass/library/jobs.hpp"
TEST_CASE("analysis admission rejects incompatible decoder snapshots") {
  beatglass::library_detail::SqliteCache cache(":memory:");
  beatglass::AnalysisIdentity identity{"hash", "engine", "config", "offline"};
  const auto rejected = beatglass::library::admit_analysis(cache, "bad-snapshot", identity, "/music/a.flac", "file-reader-v0");
  CHECK_FALSE(rejected.decoder_compatible);
  CHECK_FALSE(rejected.analysis_reserved);
  CHECK_FALSE(rejected.cached.has_value());
}
TEST_CASE("retryable recovery looks up one persisted run by ID") {
  beatglass::library_detail::SqliteCache cache(":memory:");
  const beatglass::library_detail::CacheKey key{"recovery-id", "engine", "config", "offline"};
  cache.begin("recovery-id", key, "/music/recovery.flac", beatglass::library::kFileDecoderSnapshot);
  REQUIRE(cache.publish("recovery-id", key, "stale", {}, [] { return false; }) == beatglass::library_detail::Publish::stale_input);
  const auto found = cache.retryable_run("recovery-id");
  REQUIRE(found);
  CHECK(found->source_path == "/music/recovery.flac");
  CHECK(found->decoder_snapshot == beatglass::library::kFileDecoderSnapshot);
  CHECK_FALSE(cache.retryable_run("missing-recovery-id"));
}
TEST_CASE("retryable recovery survives storage reopen") {
  const auto path = std::filesystem::temp_directory_path() / "beatglass-recovery-reopen.sqlite";
  std::filesystem::remove(path);
  const beatglass::library_detail::CacheKey key{"reopen-id", "engine", "config", "offline"};
  {
    beatglass::library_detail::SqliteCache cache(path.string());
    cache.begin("reopen-id", key, "/music/reopen.flac", beatglass::library::kFileDecoderSnapshot);
    REQUIRE(cache.publish("reopen-id", key, "stale", {}, [] { return false; }) == beatglass::library_detail::Publish::stale_input);
  }
  {
    beatglass::library_detail::SqliteCache cache(path.string());
    const auto recovered = cache.retryable_run("reopen-id");
    REQUIRE(recovered.has_value());
    CHECK(recovered->source_path == "/music/reopen.flac");
    CHECK(recovered->decoder_snapshot == beatglass::library::kFileDecoderSnapshot);
  }
  std::filesystem::remove(path);
}
TEST_CASE("analysis admission accepts a compatible snapshot cache hit") {
  beatglass::library_detail::SqliteCache cache(":memory:");
  beatglass::AnalysisIdentity identity{"hash", "engine", "config", "offline"};
  cache.begin("seed", {"hash", "engine", "config", "offline"}, "/music/a.flac", beatglass::library::kFileDecoderSnapshot);
  REQUIRE(cache.publish("seed", {"hash", "engine", "config", "offline"}, "cached", {}, [] { return true; }) == beatglass::library_detail::Publish::committed);
  const auto hit = beatglass::library::admit_analysis(cache, "unused", identity, "/music/a.flac", beatglass::library::kFileDecoderSnapshot);
  REQUIRE(hit.decoder_compatible);
  CHECK(hit.cached == std::optional<std::string>{"cached"});
  CHECK_FALSE(hit.analysis_reserved);
}

TEST_CASE("cache acceptance reuses unchanged content and invalidates content or configuration changes") {
  beatglass::library_detail::SqliteCache cache(":memory:");
  const beatglass::AnalysisIdentity unchanged{"sha256:file-v1", "engine-v1", "config-a", "offline"};
  cache.begin("initial-run", {unchanged.content_hash, unchanged.engine_version, unchanged.configuration_id, unchanged.mode}, "/music/acceptance.wav", beatglass::library::kFileDecoderSnapshot);
  REQUIRE(cache.publish("initial-run", {unchanged.content_hash, unchanged.engine_version, unchanged.configuration_id, unchanged.mode}, "analysis-v1", {}, [] { return true; }) == beatglass::library_detail::Publish::committed);

  const auto reused = beatglass::library::admit_analysis(cache, "reuse-run", unchanged, "/music/acceptance.wav", beatglass::library::kFileDecoderSnapshot);
  CHECK(reused.cached == std::optional<std::string>{"analysis-v1"});
  CHECK_FALSE(reused.analysis_reserved);

  const beatglass::AnalysisIdentity changed_content{"sha256:file-v2", "engine-v1", "config-a", "offline"};
  const auto content_miss = beatglass::library::admit_analysis(cache, "content-change", changed_content, "/music/acceptance.wav", beatglass::library::kFileDecoderSnapshot);
  CHECK_FALSE(content_miss.cached.has_value());
  CHECK(content_miss.analysis_reserved);

  const beatglass::AnalysisIdentity changed_configuration{"sha256:file-v1", "engine-v1", "config-b", "offline"};
  const auto config_miss = beatglass::library::admit_analysis(cache, "config-change", changed_configuration, "/music/acceptance.wav", beatglass::library::kFileDecoderSnapshot);
  CHECK_FALSE(config_miss.cached.has_value());
  CHECK(config_miss.analysis_reserved);
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
TEST_CASE("storage owner invalidates one cached identity without affecting another") {
  beatglass::library::StorageOwner owner;
  REQUIRE(owner.open_storage({":memory:"}));
  const beatglass::AnalysisIdentity first{"first", "engine", "config", "offline"};
  const beatglass::AnalysisIdentity second{"second", "engine", "config", "offline"};
  owner.cache()->begin("first-run", {"first", "engine", "config", "offline"});
  REQUIRE(owner.cache()->publish("first-run", {"first", "engine", "config", "offline"}, "first-result", {}, [] { return true; }) == beatglass::library_detail::Publish::committed);
  owner.cache()->begin("second-run", {"second", "engine", "config", "offline"});
  REQUIRE(owner.cache()->publish("second-run", {"second", "engine", "config", "offline"}, "second-result", {}, [] { return true; }) == beatglass::library_detail::Publish::committed);
  REQUIRE(owner.invalidate_cached(first));
  std::string result;
  CHECK_FALSE(owner.find_cached(first, result));
  REQUIRE(owner.find_cached(second, result));
  CHECK(result == "second-result");
}
TEST_CASE("invalidated identity is admitted for fresh reanalysis") {
  beatglass::library::StorageOwner owner;
  REQUIRE(owner.open_storage({":memory:"}));
  const beatglass::AnalysisIdentity identity{"reanalyze", "engine", "config", "offline"};
  const beatglass::library_detail::CacheKey key{"reanalyze", "engine", "config", "offline"};
  owner.cache()->begin("reanalyze-seed", key, "track.wav", beatglass::library::kFileDecoderSnapshot);
  REQUIRE(owner.cache()->publish("reanalyze-seed", key, "old", {}, [] { return true; }) == beatglass::library_detail::Publish::committed);
  REQUIRE(owner.invalidate_cached(identity));
  std::string result;
  CHECK_FALSE(owner.find_cached(identity, result));
  const auto admission = owner.admit_analysis("reanalyze-fresh", identity, "track.wav", beatglass::library::kFileDecoderSnapshot);
  CHECK(admission.decoder_compatible);
  CHECK_FALSE(admission.cached.has_value());
  CHECK(admission.analysis_reserved);
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
  const auto recovered = coordinator.storage().retryable_run("cancel-run");
  REQUIRE(recovered);
  CHECK(recovered->source_path == "track.wav");
}
TEST_CASE("decoder failure retains a retryable recovery record") {
  beatglass::library::AnalysisCoordinator coordinator;
  REQUIRE(coordinator.open_storage({":memory:"}));
  beatglass::library::AnalysisOperation op;
  op.source.path = "broken.wav"; op.source.content_hash = "broken-hash";
  op.identity = {"broken-hash", "engine", "config", "offline"}; op.decoder_snapshot = beatglass::library::kFileDecoderSnapshot;
  op.decode = [](std::stop_token, const auto&) -> std::string { throw std::runtime_error("decoder interrupted"); };
  op.verify = [](std::stop_token) { return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match}; };
  auto handle = coordinator.submit("decoder-failure", std::move(op));
  REQUIRE(handle);
  for (int i = 0; i < 1000 && (handle->state() == beatglass::library_detail::TrackedState::queued || handle->state() == beatglass::library_detail::TrackedState::running); ++i) std::this_thread::yield();
  CHECK(handle->state() == beatglass::library_detail::TrackedState::failed);
  const auto recovered = coordinator.storage().retryable_run("decoder-failure");
  REQUIRE(recovered);
  CHECK(recovered->source_path == "broken.wav");
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
TEST_CASE("endpoint replacement publishes only the new source epoch") {
  beatglass::library_detail::SqliteCache cache(":memory:");
  beatglass::library_detail::CacheKey old_key{"endpoint-a-epoch-1", "engine", "config", "offline"};
  beatglass::library_detail::CacheKey new_key{"endpoint-b-epoch-2", "engine", "config", "offline"};
  cache.begin("endpoint-old", old_key, "endpoint://device-a", beatglass::library::kFileDecoderSnapshot);
  REQUIRE(cache.publish("endpoint-old", old_key, "stale-device-a", {}, [] { return false; }) == beatglass::library_detail::Publish::stale_input);
  cache.begin("endpoint-new", new_key, "endpoint://device-b", beatglass::library::kFileDecoderSnapshot);
  REQUIRE(cache.publish("endpoint-new", new_key, "fresh-device-b", {}, [] { return true; }) == beatglass::library_detail::Publish::committed);
  std::string result;
  CHECK_FALSE(cache.find(old_key, result));
  REQUIRE(cache.find(new_key, result));
  CHECK(result == "fresh-device-b");
  const auto retryable = cache.retryable_runs();
  REQUIRE(retryable.size() == 1);
  CHECK(retryable.front().source_path == "endpoint://device-a");
  CHECK(retryable.front().key.content == "endpoint-a-epoch-1");
}
TEST_CASE("endpoint replacement retry state survives cache restart") {
  const auto path = std::filesystem::temp_directory_path() /
    ("beatglass-endpoint-restart-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".sqlite");
  beatglass::library_detail::CacheKey old_key{"endpoint-a-epoch-1", "engine", "config", "offline"};
  beatglass::library_detail::CacheKey new_key{"endpoint-b-epoch-2", "engine", "config", "offline"};
  {
    beatglass::library_detail::SqliteCache cache(path.string());
    cache.begin("endpoint-old", old_key, "endpoint://device-a", beatglass::library::kFileDecoderSnapshot);
    REQUIRE(cache.publish("endpoint-old", old_key, "stale", {}, [] { return false; }) == beatglass::library_detail::Publish::stale_input);
    cache.begin("endpoint-new", new_key, "endpoint://device-b", beatglass::library::kFileDecoderSnapshot);
    REQUIRE(cache.publish("endpoint-new", new_key, "fresh", {}, [] { return true; }) == beatglass::library_detail::Publish::committed);
  }
  {
    beatglass::library_detail::SqliteCache cache(path.string());
    std::string result;
    REQUIRE(cache.find(new_key, result));
    CHECK(result == "fresh");
    const auto retryable = cache.retryable_runs();
    REQUIRE(retryable.size() == 1);
    CHECK(retryable.front().source_path == "endpoint://device-a");
    CHECK(retryable.front().key.content == "endpoint-a-epoch-1");
  }
  REQUIRE(std::filesystem::remove(path));
}
TEST_CASE("coordinator resubmits persisted endpoint retry after restart") {
  const auto db = std::filesystem::temp_directory_path() / ("beatglass-coordinator-retry-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".sqlite");
  const auto source = std::filesystem::temp_directory_path() / ("beatglass-endpoint-source-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".wav");
  { std::ofstream out(source, std::ios::binary); out << "fixture"; }
  beatglass::library_detail::CacheKey key{"restart-endpoint", "engine", "config", "offline"};
  {
    beatglass::library_detail::SqliteCache cache(db.string());
    cache.begin("restart-run", key, source.string(), beatglass::library::kFileDecoderSnapshot);
    REQUIRE(cache.publish("restart-run", key, "stale", {}, [] { return false; }) == beatglass::library_detail::Publish::stale_input);
  }
  beatglass::library::AnalysisCoordinator coordinator;
  REQUIRE(coordinator.open_storage({db.string()}));
  const auto pending = coordinator.storage().retryable_runs();
  REQUIRE(pending.size() == 1);
  REQUIRE(pending.front().source_path == source.string());
  REQUIRE(pending.front().decoder_snapshot == beatglass::library::kFileDecoderSnapshot);
  beatglass::library::AnalysisOperation operation;
  operation.source.path = pending.front().source_path; operation.source.content_hash = pending.front().key.content;
  operation.identity = {pending.front().key.content, pending.front().key.engine, pending.front().key.config, pending.front().key.mode};
  operation.decoder_snapshot = pending.front().decoder_snapshot;
  operation.serialize = [] { return std::string("recovered"); };
  operation.verify = [](std::stop_token) { return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match}; };
  const auto handle = coordinator.resubmit_retryable(pending.front().id, std::move(operation));
  REQUIRE(handle);
  for (int i = 0; i < 1000 && (handle->state() == beatglass::library_detail::TrackedState::queued || handle->state() == beatglass::library_detail::TrackedState::running); ++i) std::this_thread::yield();
  REQUIRE(handle->state() == beatglass::library_detail::TrackedState::completed);
  std::string result;
  REQUIRE(coordinator.storage().cache()->find(key, result));
  CHECK(result == "recovered");
  CHECK(coordinator.storage().retryable_runs().empty());
  coordinator.close_storage();
  REQUIRE(std::filesystem::remove(db));
  REQUIRE(std::filesystem::remove(source));
}
TEST_CASE("coordinator rejects persisted retry with mismatched snapshot and keeps it") {
  beatglass::library::AnalysisCoordinator coordinator;
  REQUIRE(coordinator.open_storage({":memory:"}));
  const beatglass::library_detail::CacheKey key{"mismatch-retry", "engine", "config", "offline"};
  coordinator.storage().cache()->begin("mismatch-retry", key, "missing.wav", beatglass::library::kFileDecoderSnapshot);
  REQUIRE(coordinator.storage().cache()->publish("mismatch-retry", key, "stale", {}, [] { return false; }) == beatglass::library_detail::Publish::stale_input);
  beatglass::library::AnalysisOperation operation;
  operation.source.path = "missing.wav"; operation.source.content_hash = key.content;
  operation.identity = {key.content, key.engine, key.config, key.mode}; operation.decoder_snapshot = "file-reader-v0";
  operation.serialize = [] { return std::string("must-not-run"); };
  operation.verify = [](std::stop_token) { return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match}; };
  CHECK_FALSE(coordinator.resubmit_retryable("mismatch-retry", std::move(operation)));
  REQUIRE(coordinator.storage().retryable_runs().size() == 1);
}
TEST_CASE("coordinator rejects persisted retry from a stale source epoch") {
  beatglass::library::AnalysisCoordinator coordinator;
  REQUIRE(coordinator.open_storage({":memory:"}));
  const beatglass::library_detail::CacheKey key{"epoch-1", "engine", "config", "offline"};
  coordinator.storage().cache()->begin("epoch-retry", key, "endpoint://device", beatglass::library::kFileDecoderSnapshot);
  REQUIRE(coordinator.storage().cache()->publish("epoch-retry", key, "stale", {}, [] { return false; }) == beatglass::library_detail::Publish::stale_input);
  beatglass::library::AnalysisOperation operation;
  operation.source.path = "endpoint://device"; operation.source.content_hash = "epoch-2";
  operation.identity = {"epoch-2", key.engine, key.config, key.mode}; operation.decoder_snapshot = beatglass::library::kFileDecoderSnapshot;
  operation.serialize = [] { return std::string("must-not-run"); };
  operation.verify = [](std::stop_token) { return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match}; };
  CHECK_FALSE(coordinator.resubmit_retryable("epoch-retry", std::move(operation)));
  REQUIRE(coordinator.storage().retryable_runs().size() == 1);
  CHECK(coordinator.storage().retryable_runs().front().key.content == "epoch-1");
}
TEST_CASE("coordinator explains stale source identity before retry") {
  beatglass::library::AnalysisCoordinator coordinator;
  REQUIRE(coordinator.open_storage({":memory:"}));
  const beatglass::library_detail::CacheKey key{"epoch-1", "engine", "config", "offline"};
  coordinator.storage().cache()->begin("identity-retry", key, "endpoint://device", beatglass::library::kFileDecoderSnapshot);
  REQUIRE(coordinator.storage().cache()->publish("identity-retry", key, "stale", {}, [] { return false; }) == beatglass::library_detail::Publish::stale_input);
  beatglass::library::AnalysisOperation operation;
  operation.source.path = "endpoint://device"; operation.source.content_hash = "epoch-2";
  operation.identity = {"epoch-2", key.engine, key.config, key.mode}; operation.decoder_snapshot = beatglass::library::kFileDecoderSnapshot;
  CHECK(coordinator.retry_rejection_reason("identity-retry", operation) == beatglass::library::RetryRejectionReason::stale_source_identity);
  CHECK(std::string(beatglass::library::retry_rejection_message(beatglass::library::RetryRejectionReason::stale_source_identity)) == "stale source identity");
}
TEST_CASE("stale retry rejection reason survives coordinator restart") {
  const auto db = std::filesystem::temp_directory_path() / ("beatglass-stale-reason-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".sqlite");
  const beatglass::library_detail::CacheKey key{"restart-epoch-1", "engine", "config", "offline"};
  {
    beatglass::library_detail::SqliteCache cache(db.string());
    cache.begin("stale-reason", key, "endpoint://device", beatglass::library::kFileDecoderSnapshot);
    REQUIRE(cache.publish("stale-reason", key, "stale", {}, [] { return false; }) == beatglass::library_detail::Publish::stale_input);
  }
  beatglass::library::AnalysisCoordinator coordinator;
  REQUIRE(coordinator.open_storage({db.string()}));
  beatglass::library::AnalysisOperation operation;
  operation.source.path = "endpoint://device"; operation.source.content_hash = "restart-epoch-2";
  operation.identity = {"restart-epoch-2", key.engine, key.config, key.mode}; operation.decoder_snapshot = beatglass::library::kFileDecoderSnapshot;
  operation.serialize = [] { return std::string("must-not-run"); };
  operation.verify = [](std::stop_token) { return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match}; };
  CHECK(coordinator.retry_rejection_reason("stale-reason", operation) == beatglass::library::RetryRejectionReason::stale_source_identity);
  CHECK(std::string(beatglass::library::retry_rejection_message(beatglass::library::RetryRejectionReason::stale_source_identity)) == "stale source identity");
  REQUIRE(coordinator.storage().retryable_runs().size() == 1);
  CHECK(coordinator.storage().retryable_runs().front().source_path == "endpoint://device");
  coordinator.close_storage();
  REQUIRE(std::filesystem::remove(db));
}
TEST_CASE("decoder mismatch and missing source reasons survive restart") {
  const auto db = std::filesystem::temp_directory_path() / ("beatglass-rejection-reasons-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".sqlite");
  const beatglass::library_detail::CacheKey decoder_key{"decoder-restart", "engine", "config", "offline"};
  const beatglass::library_detail::CacheKey missing_key{"missing-restart", "engine", "config", "offline"};
  {
    beatglass::library_detail::SqliteCache cache(db.string());
    cache.begin("decoder-rejection", decoder_key, "endpoint://decoder", beatglass::library::kFileDecoderSnapshot);
    REQUIRE(cache.publish("decoder-rejection", decoder_key, "stale", {}, [] { return false; }) == beatglass::library_detail::Publish::stale_input);
    cache.begin("missing-rejection", missing_key, "", beatglass::library::kFileDecoderSnapshot);
    REQUIRE(cache.publish("missing-rejection", missing_key, "stale", {}, [] { return false; }) == beatglass::library_detail::Publish::stale_input);
  }
  beatglass::library::AnalysisCoordinator coordinator;
  REQUIRE(coordinator.open_storage({db.string()}));
  beatglass::library::AnalysisOperation decoder_operation;
  decoder_operation.source.path = "endpoint://decoder"; decoder_operation.source.content_hash = decoder_key.content;
  decoder_operation.identity = {decoder_key.content, decoder_key.engine, decoder_key.config, decoder_key.mode}; decoder_operation.decoder_snapshot = "file-reader-v0";
  CHECK(coordinator.retry_rejection_reason("decoder-rejection", decoder_operation) == beatglass::library::RetryRejectionReason::incompatible_decoder_snapshot);
  CHECK(std::string(beatglass::library::retry_rejection_message(beatglass::library::RetryRejectionReason::incompatible_decoder_snapshot)) == "incompatible decoder snapshot");
  beatglass::library::AnalysisOperation missing_operation;
  missing_operation.source.path = ""; missing_operation.source.content_hash = missing_key.content;
  missing_operation.identity = {missing_key.content, missing_key.engine, missing_key.config, missing_key.mode}; missing_operation.decoder_snapshot = beatglass::library::kFileDecoderSnapshot;
  CHECK(coordinator.retry_rejection_reason("missing-rejection", missing_operation) == beatglass::library::RetryRejectionReason::missing_source_snapshot);
  CHECK(std::string(beatglass::library::retry_rejection_message(beatglass::library::RetryRejectionReason::missing_source_snapshot)) == "persisted source snapshot unavailable");
  REQUIRE(coordinator.storage().retryable_runs().size() == 2);
  coordinator.close_storage();
  REQUIRE(std::filesystem::remove(db));
}
TEST_CASE("resolved retry diagnostics clear after recovery or discard") {
  beatglass::library_detail::SqliteCache cache(":memory:");
  const beatglass::library_detail::CacheKey recovered_key{"resolved", "engine", "config", "offline"};
  cache.begin("resolved-run", recovered_key, "endpoint://resolved", beatglass::library::kFileDecoderSnapshot);
  REQUIRE(cache.publish("resolved-run", recovered_key, "stale", {}, [] { return false; }) == beatglass::library_detail::Publish::stale_input);
  REQUIRE(cache.retryable_runs().size() == 1);
  cache.begin("resolved-run", recovered_key, "endpoint://resolved", beatglass::library::kFileDecoderSnapshot);
  REQUIRE(cache.publish("resolved-run", recovered_key, "fresh", {}, [] { return true; }) == beatglass::library_detail::Publish::committed);
  CHECK(cache.retryable_runs().empty());
  std::string result;
  REQUIRE(cache.find(recovered_key, result));
  CHECK(result == "fresh");

  const beatglass::library_detail::CacheKey discarded_key{"discarded", "engine", "config", "offline"};
  cache.begin("discarded-run", discarded_key, "endpoint://discarded", beatglass::library::kFileDecoderSnapshot);
  REQUIRE(cache.publish("discarded-run", discarded_key, "stale", {}, [] { return false; }) == beatglass::library_detail::Publish::stale_input);
  REQUIRE(cache.discard_retryable("discarded-run"));
  CHECK(cache.retryable_runs().empty());
}
TEST_CASE("coordinator missing source retry fails closed without a cache result") {
  beatglass::library::AnalysisCoordinator coordinator;
  REQUIRE(coordinator.open_storage({":memory:"}));
  beatglass::library::AnalysisOperation operation;
  operation.source.path = "missing-source.wav"; operation.source.content_hash = "missing-source";
  operation.identity = {"missing-source", "engine", "config", "offline"}; operation.decoder_snapshot = beatglass::library::kFileDecoderSnapshot;
  operation.serialize = [] { return std::string("must-not-publish"); };
  operation.verify = [](std::stop_token) { return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::unverifiable, "source file is unavailable"}; };
  const auto handle = coordinator.submit("missing-source-run", std::move(operation));
  REQUIRE(handle);
  for (int i = 0; i < 1000 && (handle->state() == beatglass::library_detail::TrackedState::queued || handle->state() == beatglass::library_detail::TrackedState::running); ++i) std::this_thread::yield();
  CHECK(handle->state() == beatglass::library_detail::TrackedState::failed);
  std::string result;
  CHECK_FALSE(coordinator.storage().cache()->find({"missing-source", "engine", "config", "offline"}, result));
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
  cache.begin("run", {"sha256:test", "engine", "config", "offline"});
  REQUIRE(beatglass::library::publish_operation(cache, "run", op, stop.get_token()) == beatglass::library_detail::Publish::cancelled);
  CHECK_FALSE(serialized); CHECK_FALSE(verified);
  REQUIRE(cache.retryable_runs().size() == 1);
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
