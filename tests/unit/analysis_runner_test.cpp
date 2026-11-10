#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/analysis_runner.hpp"
#include <chrono>
#include <atomic>
#include <thread>
#include <vector>
using namespace beatglass::library;
using namespace beatglass::library_detail;
namespace { AnalysisOperation op(bool valid) { AnalysisOperation o; o.source.path="track.wav"; o.source.content_hash="sha256:test"; o.identity.content_hash="sha256:test"; o.identity.engine_version="engine"; o.identity.configuration_id="config"; o.identity.mode="offline"; o.decoder_snapshot="libsndfile-test"; o.source.size_bytes=12; o.serialize=[] { return std::string("result"); }; o.verify=[valid](std::stop_token){ return beatglass::FileVerificationResult{valid ? beatglass::FileVerificationOutcome::match : beatglass::FileVerificationOutcome::changed}; }; return o; } }
namespace { void wait_for_terminal(const std::shared_ptr<JobHandle>& handle) { for(int i=0;i<10000; ++i) { const auto state=handle->state(); if(state != TrackedState::queued && state != TrackedState::running) return; std::this_thread::sleep_for(std::chrono::microseconds(100)); } } }
TEST_CASE("analysis runner cancellation never publishes a result") { JobExecutor executor(1); AnalysisRunner runner(executor); auto operation=op(true); operation.serialize=[] { return std::string("should not publish"); }; operation.verify=[](std::stop_token stop) { while(!stop.stop_requested()) std::this_thread::yield(); return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::cancelled}; }; auto h=runner.submit(std::move(operation)); REQUIRE(h); h->cancel(); wait_for_terminal(h); CHECK(h->state()==TrackedState::cancelled); CHECK(h->frames()==0); executor.shutdown(); }
TEST_CASE("cancelled queued analysis releases executor capacity") {
  JobExecutor executor(1); AnalysisRunner runner(executor);
  auto first = runner.submit(op(true)); REQUIRE(first); first->cancel();
  auto second = runner.submit(op(true)); CHECK(second); executor.shutdown();
}
TEST_CASE("analysis runner converts verifier exceptions into a failed job") { JobExecutor executor(1); AnalysisRunner runner(executor); auto operation=op(true); operation.verify=[](std::stop_token) -> beatglass::FileVerificationResult { throw std::runtime_error("source disappeared"); }; auto h=runner.submit(std::move(operation)); REQUIRE(h); wait_for_terminal(h); CHECK(h->state()==TrackedState::failed); CHECK(h->frames()==0); executor.shutdown(); }
TEST_CASE("analysis runner completes verified work and reports progress") { JobExecutor executor(1); AnalysisRunner runner(executor); auto h=runner.submit(op(true)); REQUIRE(h); wait_for_terminal(h); CHECK(h->state()==TrackedState::completed); CHECK(h->frames()==12); executor.shutdown(); }
TEST_CASE("analysis runner publishes serialized result before completion") {
  JobExecutor executor(1); AnalysisRunner runner(executor); std::string published;
  auto operation = op(true); operation.publish = [&](const std::string& result) { published = result; };
  auto h = runner.submit(std::move(operation)); REQUIRE(h); wait_for_terminal(h);
  CHECK(h->state() == TrackedState::completed); CHECK(published == "result"); executor.shutdown();
}
TEST_CASE("analysis runner forwards decoder progress into the tracked handle") {
  JobExecutor executor(1); AnalysisRunner runner(executor); std::vector<std::uint64_t> reported;
  auto operation = op(true); operation.progress = [&](std::uint64_t done, std::uint64_t) { reported.push_back(done); };
  auto h = runner.submit(std::move(operation)); REQUIRE(h); wait_for_terminal(h);
  REQUIRE(reported.size() == 2); CHECK(reported.front() == 0); CHECK(reported.back() == 12); CHECK(h->frames() == 12);
  executor.shutdown();
}
TEST_CASE("analysis runner hands decoded result and incremental progress to publication") {
  JobExecutor executor(1); AnalysisRunner runner(executor); std::string published; std::vector<std::uint64_t> observed;
  auto operation = op(true); operation.serialize = {}; operation.decode = [&](std::stop_token, const auto& progress) { progress(4); progress(9); return std::string("decoded"); };
  operation.publish = [&](const std::string& result) { published = result; };
  auto h = runner.submit(std::move(operation)); REQUIRE(h); wait_for_terminal(h);
  CHECK(h->state() == TrackedState::completed); CHECK(h->frames() == 12); CHECK(published == "decoded"); executor.shutdown();
}
TEST_CASE("analysis runner fails closed when verification rejects input") { JobExecutor executor(1); AnalysisRunner runner(executor); auto h=runner.submit(op(false)); REQUIRE(h); wait_for_terminal(h); CHECK(h->state()==TrackedState::failed); executor.shutdown(); }
TEST_CASE("analysis runner rejects operations without source attribution") { JobExecutor executor(1); AnalysisRunner runner(executor); auto missing=op(true); missing.source.path.clear(); auto h=runner.submit(std::move(missing)); REQUIRE(h); wait_for_terminal(h); CHECK(h->state()==TrackedState::failed); executor.shutdown(); }
TEST_CASE("analysis runner fails closed on empty serialization") { JobExecutor executor(1); AnalysisRunner runner(executor); auto empty=op(true); empty.serialize=[] { return std::string{}; }; auto h=runner.submit(std::move(empty)); REQUIRE(h); wait_for_terminal(h); CHECK(h->state()==TrackedState::failed); executor.shutdown(); }
TEST_CASE("invalid analysis operation does not invoke verifier") { JobExecutor executor(1); AnalysisRunner runner(executor); auto invalid=op(true); bool verified=false; invalid.source.content_hash="wrong"; invalid.verify=[&](std::stop_token){ verified=true; return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match}; }; auto h=runner.submit(std::move(invalid)); REQUIRE(h); wait_for_terminal(h); CHECK(h->state()==TrackedState::failed); CHECK_FALSE(verified); executor.shutdown(); }
TEST_CASE("analysis operation validity captures decoder submit requirements") { auto missing=op(true); missing.source.content_hash="different"; CHECK_FALSE(missing.valid()); }
TEST_CASE("analysis operation validity rejects embedded NUL identity data") { auto invalid=op(true); invalid.identity.configuration_id.assign("config\0suffix", 13); CHECK_FALSE(invalid.valid()); }
TEST_CASE("analysis runner retains cancellable worker operation") {
  beatglass::library_detail::JobExecutor executor(1); beatglass::library::AnalysisRunner runner(executor); std::stop_source gate;
  beatglass::library::AnalysisOperation op; op.source.size_bytes=10; op.identity.mode="offline"; op.serialize=[] { return std::string("x"); }; op.verify=[](std::stop_token){ return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match,{}}; };
  auto handle=runner.submit(std::move(op)); REQUIRE(handle); handle->cancel(); wait_for_terminal(handle); REQUIRE((handle->state()==beatglass::library_detail::TrackedState::cancelled || handle->state()==beatglass::library_detail::TrackedState::completed));
}
TEST_CASE("analysis runner submits bounded batches in source order") {
  beatglass::library_detail::JobExecutor executor(1); beatglass::library::AnalysisRunner runner(executor);
  std::atomic_bool release{false};
  auto blocked = op(true);
  blocked.verify = [&](std::stop_token stop) {
    while (!release.load(std::memory_order_acquire) && !stop.stop_requested()) std::this_thread::yield();
    return beatglass::FileVerificationResult{stop.stop_requested() ? beatglass::FileVerificationOutcome::cancelled : beatglass::FileVerificationOutcome::match};
  };
  auto active = runner.submit(blocked);
  REQUIRE(active);
  for (int i = 0; i < 1000 && active->state() != TrackedState::running; ++i) std::this_thread::yield();
  REQUIRE(active->state() == TrackedState::running);
  std::vector<AnalysisOperation> operations{blocked, blocked};
  auto handles = runner.submit_batch(std::move(operations));
  REQUIRE(handles.size() == 1);
  release.store(true, std::memory_order_release);
  wait_for_terminal(active);
  for (const auto& handle : handles) { REQUIRE(handle); wait_for_terminal(handle); CHECK(handle->state() == TrackedState::completed); }
  executor.shutdown();
}
TEST_CASE("analysis operation validity bounds cache identity fields") { auto invalid=op(true); invalid.identity.mode="unknown"; CHECK_FALSE(invalid.valid()); invalid=op(true); invalid.identity.configuration_id.assign(4097, 'x'); CHECK_FALSE(invalid.valid()); }
TEST_CASE("analysis operation validity requires serialization and verification callbacks") { auto missing=op(true); missing.serialize={}; CHECK_FALSE(missing.valid()); missing=op(true); missing.verify={}; CHECK_FALSE(missing.valid()); }
TEST_CASE("retryable analysis reuses source identity after a failed attempt") {
  JobExecutor executor(1); AnalysisRunner runner(executor); int attempts = 0;
  auto operation = op(true);
  operation.verify = [&](std::stop_token) { ++attempts; return beatglass::FileVerificationResult{attempts == 1 ? beatglass::FileVerificationOutcome::changed : beatglass::FileVerificationOutcome::match}; };
  auto retryable = runner.submit_retryable(std::move(operation)); REQUIRE(retryable);
  for (int i = 0; i < 1000 && (retryable->state() == TrackedState::queued || retryable->state() == TrackedState::running); ++i) std::this_thread::yield();
  REQUIRE(retryable->state() == TrackedState::failed);
  REQUIRE(retryable->retry(1));
  for (int i = 0; i < 1000 && (retryable->state() == TrackedState::queued || retryable->state() == TrackedState::running); ++i) std::this_thread::yield();
  CHECK(retryable->state() == TrackedState::completed); CHECK(attempts == 2); CHECK(retryable->frames() == 12);
  executor.shutdown();
}
TEST_CASE("decoder-only analysis publishes its decoded result") {
  JobExecutor executor(1); AnalysisRunner runner(executor); std::string published;
  AnalysisOperation operation; operation.source.path = "track.wav"; operation.source.content_hash = "sha256:test";
  operation.identity.content_hash = "sha256:test"; operation.identity.engine_version = "engine";
  operation.identity.configuration_id = "config"; operation.identity.mode = "offline"; operation.source.size_bytes = 12;
  operation.decoder_snapshot = "libsndfile-test";
  operation.decode = [](std::stop_token stop, const auto& progress) { if (stop.stop_requested()) return std::string{}; progress(12); return std::string("decoded_frames=12"); };
  operation.verify = [](std::stop_token) { return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match}; };
  operation.publish = [&](const std::string& result) { published = result; };
  auto handle = runner.submit(std::move(operation)); REQUIRE(handle); wait_for_terminal(handle);
  CHECK(handle->state() == TrackedState::completed); CHECK(published == "decoded_frames=12"); CHECK(handle->frames() == 12);
  executor.shutdown();
}
