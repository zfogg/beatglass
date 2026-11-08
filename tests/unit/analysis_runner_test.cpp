#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/analysis_runner.hpp"
#include <thread>
using namespace beatglass::library;
using namespace beatglass::library_detail;
namespace { AnalysisOperation op(bool valid) { AnalysisOperation o; o.source.size_bytes=12; o.serialize=[] { return std::string("result"); }; o.verify=[valid](std::stop_token){ return FileVerificationResult{valid ? FileVerificationOutcome::match : FileVerificationOutcome::changed}; }; return o; } }
TEST_CASE("analysis runner completes verified work and reports progress") { JobExecutor executor(1); AnalysisRunner runner(executor); auto h=runner.submit(op(true)); REQUIRE(h); for(int i=0;i<100 && h->state()==TrackedState::queued;++i) std::this_thread::yield(); executor.shutdown(); CHECK(h->state()==TrackedState::completed); CHECK(h->frames()==12); }
TEST_CASE("analysis runner fails closed when verification rejects input") { JobExecutor executor(1); AnalysisRunner runner(executor); auto h=runner.submit(op(false)); REQUIRE(h); executor.shutdown(); CHECK(h->state()==TrackedState::failed); }
