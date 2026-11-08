#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/analysis_runner.hpp"
#include <chrono>
#include <thread>
using namespace beatglass::library;
using namespace beatglass::library_detail;
namespace { AnalysisOperation op(bool valid) { AnalysisOperation o; o.source.path="track.wav"; o.source.content_hash="sha256:test"; o.identity.content_hash="sha256:test"; o.source.size_bytes=12; o.serialize=[] { return std::string("result"); }; o.verify=[valid](std::stop_token){ return beatglass::FileVerificationResult{valid ? beatglass::FileVerificationOutcome::match : beatglass::FileVerificationOutcome::changed}; }; return o; } }
namespace { void wait_for_terminal(const std::shared_ptr<JobHandle>& handle) { for(int i=0;i<1000; ++i) { const auto state=handle->state(); if(state != TrackedState::queued && state != TrackedState::running) return; std::this_thread::yield(); } } }
TEST_CASE("analysis runner cancellation never publishes a result") { JobExecutor executor(1); AnalysisRunner runner(executor); auto operation=op(true); operation.serialize=[] { return std::string("should not publish"); }; operation.verify=[](std::stop_token stop) { while(!stop.stop_requested()) std::this_thread::yield(); return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::cancelled}; }; auto h=runner.submit(std::move(operation)); REQUIRE(h); h->cancel(); wait_for_terminal(h); CHECK(h->state()==TrackedState::cancelled); CHECK(h->frames()==0); executor.shutdown(); }
TEST_CASE("analysis runner completes verified work and reports progress") { JobExecutor executor(1); AnalysisRunner runner(executor); auto h=runner.submit(op(true)); REQUIRE(h); wait_for_terminal(h); CHECK(h->state()==TrackedState::completed); CHECK(h->frames()==12); executor.shutdown(); }
TEST_CASE("analysis runner fails closed when verification rejects input") { JobExecutor executor(1); AnalysisRunner runner(executor); auto h=runner.submit(op(false)); REQUIRE(h); wait_for_terminal(h); CHECK(h->state()==TrackedState::failed); executor.shutdown(); }
TEST_CASE("analysis runner rejects operations without source attribution") { JobExecutor executor(1); AnalysisRunner runner(executor); auto missing=op(true); missing.source.path.clear(); auto h=runner.submit(std::move(missing)); REQUIRE(h); wait_for_terminal(h); CHECK(h->state()==TrackedState::failed); executor.shutdown(); }
TEST_CASE("analysis runner fails closed on empty serialization") { JobExecutor executor(1); AnalysisRunner runner(executor); auto empty=op(true); empty.serialize=[] { return std::string{}; }; auto h=runner.submit(std::move(empty)); REQUIRE(h); wait_for_terminal(h); CHECK(h->state()==TrackedState::failed); executor.shutdown(); }
TEST_CASE("analysis runner retains cancellable worker operation") {
  beatglass::library_detail::JobExecutor executor(1); beatglass::library::AnalysisRunner runner(executor); std::stop_source gate;
  beatglass::library::AnalysisOperation op; op.source.size_bytes=10; op.identity.mode="offline"; op.serialize=[] { return std::string("x"); }; op.verify=[](std::stop_token){ return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match,{}}; };
  auto handle=runner.submit(std::move(op)); REQUIRE(handle); handle->cancel(); REQUIRE((handle->state()==beatglass::library_detail::TrackedState::cancelled || handle->state()==beatglass::library_detail::TrackedState::completed));
}
