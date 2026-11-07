#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/detail/tempo_analysis_store.hpp"
#include <chrono>
#include <filesystem>
using namespace beatglass;
using beatglass::library_detail::TempoAnalysisStore;
using beatglass::library_detail::CollectionEntry;
using beatglass::library_detail::CollectionOrder;
namespace {
struct TempDb {
  std::filesystem::path path = std::filesystem::temp_directory_path() / ("beatglass-typed-store-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".sqlite");
  ~TempDb() { std::error_code ignored; std::filesystem::remove(path, ignored); }
};
struct Verifier : FileVersionVerifier {
  FileVerificationOutcome outcome{FileVerificationOutcome::match}; std::stop_source* cancel{}; int calls{};
  FileVerificationResult verify() override { ++calls; if (cancel) cancel->request_stop(); return {outcome, "verification diagnostic"}; }
};
CachedAnalysis sample() {
  CachedAnalysis item; item.identity = {"sha256:input", "engine-v1", "config-v1", "offline"};
  item.result.state = TrackingState::tracking; item.result.bpm = 123.25f; item.result.observed_through = {3, 512};
  item.result.publication_monotonic_ns = 42; item.result.analysis_delay_frames = 12;
  item.result.estimator_version = "engine-v1"; item.result.configuration_id = "config-v1"; return item;
}
FileVersion version() { FileVersion f; f.content_hash = "sha256:input"; return f; }
}
TEST_CASE("typed tempo store persists snapshots and distinguishes misses") {
  TempDb db; auto item = sample(); auto file = version(); Verifier verifier;
  {
    TempoAnalysisStore store(db.path.string());
    REQUIRE(store.find(file, item.identity, {}).outcome == FindOutcome::miss);
    REQUIRE(store.publish(file, item, "run-1", verifier, {}).outcome == PublishOutcome::committed);
    CollectionEntry entry; entry.path = "example.wav"; entry.title = "Example"; entry.content_hash = item.identity.content_hash; store.put_entry(entry);
    REQUIRE(store.entries({"example", CollectionOrder::title, false, 10, 0}).size() == 1);
    REQUIRE(verifier.calls == 1);
  }
  {
    TempoAnalysisStore store(db.path.string()); REQUIRE(store.entry("example.wav"));
    auto hit = store.find(file, item.identity, {});
    REQUIRE(hit.outcome == FindOutcome::hit); REQUIRE(hit.result);
    REQUIRE(hit.result->result.bpm == item.result.bpm); REQUIRE(hit.result->result.observed_through.frame == 512);
    REQUIRE(hit.result->result.publication_monotonic_ns == 42); REQUIRE(hit.result->result.analysis_delay_frames == 12);
    REQUIRE(store.recover("run-1").outcome == RecoveryOutcome::committed);
    REQUIRE(store.recover("absent").outcome == RecoveryOutcome::not_found);
    auto changed = item.identity; changed.configuration_id = "new";
    REQUIRE(store.find(file, changed, {}).outcome == FindOutcome::miss);
  }
}
TEST_CASE("typed tempo store keeps old cache on changed input cancellation and errors") {
  TempDb db; TempoAnalysisStore store(db.path.string()); auto item = sample(); auto file = version(); Verifier verifier;
  REQUIRE(store.publish(file, item, "old", verifier, {}).outcome == PublishOutcome::committed);
  verifier.outcome = FileVerificationOutcome::changed;
  REQUIRE(store.publish(file, item, "changed", verifier, {}).outcome == PublishOutcome::stale_input);
  verifier.outcome = FileVerificationOutcome::unverifiable;
  REQUIRE(store.publish(file, item, "unverifiable", verifier, {}).outcome == PublishOutcome::stale_input);
  verifier.outcome = FileVerificationOutcome::error;
  REQUIRE(store.publish(file, item, "error", verifier, {}).outcome == PublishOutcome::error);
  std::stop_source stop; verifier.outcome = FileVerificationOutcome::match; verifier.cancel = &stop;
  REQUIRE(store.publish(file, item, "cancel", verifier, stop.get_token()).outcome == PublishOutcome::cancelled);
  auto hit = store.find(file, item.identity, {}); REQUIRE(hit.outcome == FindOutcome::hit); REQUIRE(hit.result->result.bpm == item.result.bpm);
  REQUIRE(store.find(file, item.identity, stop.get_token()).outcome == FindOutcome::cancelled);
  REQUIRE(store.recover("changed").outcome == RecoveryOutcome::not_found);
}
TEST_CASE("typed tempo store rejects data it cannot preserve and mismatched identity") {
  TempDb db; TempoAnalysisStore store(db.path.string()); auto item = sample(); auto file = version(); Verifier verifier;
  item.result.key_candidates.push_back({"C", .5f});
  REQUIRE(store.publish(file, item, "unsupported", verifier, {}).outcome == PublishOutcome::error); REQUIRE(verifier.calls == 0);
  item = sample(); file.content_hash = "different";
  REQUIRE(store.publish(file, item, "wrong", verifier, {}).outcome == PublishOutcome::error);
  REQUIRE(store.find(file, item.identity, {}).outcome == FindOutcome::error);
}

TEST_CASE("typed snapshot cache does not misread older private tempo blobs") {
  TempDb db; auto item = sample(); auto file = version();
  { beatglass::library_detail::SqliteCache old(db.path.string());
    beatglass::library_detail::CacheKey key{item.identity.content_hash, item.identity.engine_version, item.identity.configuration_id, item.identity.mode};
    old.begin("legacy", key); REQUIRE(old.publish("legacy", key, "tempo-v1 legacy", {}, [] { return true; }) == beatglass::library_detail::Publish::committed); }
  { TempoAnalysisStore store(db.path.string()); Verifier verifier;
    REQUIRE(store.find(file, item.identity, {}).outcome == FindOutcome::miss);
    REQUIRE(store.publish(file, item, "new", verifier, {}).outcome == PublishOutcome::committed);
    REQUIRE(store.find(file, item.identity, {}).outcome == FindOutcome::hit); }
  { beatglass::library_detail::SqliteCache old(db.path.string()); std::string value;
    REQUIRE(old.find({item.identity.content_hash, item.identity.engine_version, item.identity.configuration_id, item.identity.mode}, value)); REQUIRE(value == "tempo-v1 legacy"); }
}
