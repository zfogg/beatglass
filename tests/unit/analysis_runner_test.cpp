#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/analysis_runner.hpp"
#include <thread>
using namespace beatglass::library;
using namespace beatglass::library_detail;
namespace { AnalysisOperation op(bool valid) { AnalysisOperation o; o.source.size_bytes=12; o.serialize=[] { return std::string("result"); }; o.verify=[valid](std::stop_token){ return beatglass::FileVerificationResult{valid ? beatglass::FileVerificationOutcome::match : beatglass::FileVerificationOutcome::changed}; }; return o; } }
namespace { void wait_for_terminal(const std::shared_ptr<JobHandle>& handle) { for(int i=0;i<1000; ++i) { const auto state=handle->state(); if(state != TrackedState::queued && state != TrackedState::running) return; std::this_thread::yield(); } } }
TEST_CASE("analysis runner completes verified work and reports progress") { JobExecutor executor(1); AnalysisRunner runner(executor); auto h=runner.submit(op(true)); REQUIRE(h); wait_for_terminal(h); CHECK(h->state()==TrackedState::completed); CHECK(h->frames()==12); executor.shutdown(); }
TEST_CASE("analysis runner fails closed when verification rejects input") { JobExecutor executor(1); AnalysisRunner runner(executor); auto h=runner.submit(op(false)); REQUIRE(h); wait_for_terminal(h); CHECK(h->state()==TrackedState::failed); executor.shutdown(); }
TEST_CASE("analysis runner fails closed on empty serialization") { JobExecutor executor(1); AnalysisRunner runner(executor); auto empty=op(true); empty.serialize=[] { return std::string{}; }; auto h=runner.submit(std::move(empty)); REQUIRE(h); wait_for_terminal(h); CHECK(h->state()==TrackedState::failed); executor.shutdown(); }
