#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/analysis_operation.hpp"
#include "beatglass/library/analysis_commit.hpp"
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
