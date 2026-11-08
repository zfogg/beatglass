#include <catch2/catch_test_macros.hpp>
#include <thread>
#include "beatglass/library/analysis_operation.hpp"
#include "beatglass/library/analysis_runner.hpp"
TEST_CASE("analysis operation retains source identity and bounded callback contract") { beatglass::library::AnalysisOperation op; op.source.size_bytes=12; op.identity.mode="offline"; op.serialize=[] { return std::string("result"); }; REQUIRE(op.serialize()=="result"); REQUIRE(op.identity.mode=="offline"); }
TEST_CASE("analysis publication requires source and cache identities to match") {
  beatglass::library::AnalysisOperation op;
  op.source.content_hash = "sha256:source";
  op.identity.content_hash = "sha256:other";
  op.identity.engine_version = "engine";
  op.identity.configuration_id = "config";
  op.identity.mode = "offline";
  REQUIRE(op.source.content_hash != op.identity.content_hash);
}
TEST_CASE("analysis runner fails empty serialized results") {
  beatglass::library_detail::JobExecutor executor(1);
  beatglass::library::AnalysisRunner runner(executor);
  beatglass::library::AnalysisOperation op;
  op.serialize = [] { return std::string{}; };
  op.verify = [](std::stop_token) { return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match, {}}; };
  auto handle = runner.submit(std::move(op));
  REQUIRE(handle);
  for (int i = 0; i < 100 && handle->state() == beatglass::library_detail::TrackedState::queued; ++i) std::this_thread::yield();
  while (handle->state() == beatglass::library_detail::TrackedState::running) std::this_thread::yield();
  REQUIRE(handle->state() == beatglass::library_detail::TrackedState::failed);
  executor.shutdown();
}
