#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/job_lifecycle.hpp"
#include "beatglass/library/store.hpp"
using namespace beatglass;
TEST_CASE("job progress is monotonic and bounded when total is known") {
  JobLifecycle job(100); CHECK_FALSE(job.progress(1)); REQUIRE(job.start());
  REQUIRE(job.progress(20)); CHECK_FALSE(job.progress(19)); CHECK_FALSE(job.progress(101));
  CHECK(job.snapshot().frames_processed == 20); CHECK_FALSE(job.complete());
  REQUIRE(job.progress(100)); REQUIRE(job.complete()); CHECK_FALSE(job.cancel()); CHECK_FALSE(job.fail());
}
TEST_CASE("job cancellation and completion are ordered terminal decisions") {
  JobLifecycle cancelled; REQUIRE(cancelled.cancel()); CHECK_FALSE(cancelled.start()); CHECK_FALSE(cancelled.complete());
  JobLifecycle running; REQUIRE(running.start()); REQUIRE(running.progress(12)); REQUIRE(running.cancel());
  CHECK_FALSE(running.complete()); CHECK_FALSE(running.progress(13)); CHECK(running.snapshot().frames_processed == 12);
  JobLifecycle unknown; REQUIRE(unknown.start()); REQUIRE(unknown.progress(12)); REQUIRE(unknown.complete()); CHECK_FALSE(unknown.cancel());
  JobLifecycle empty(0); REQUIRE(empty.start()); REQUIRE(empty.complete());
  JobLifecycle failed; REQUIRE(failed.fail()); CHECK_FALSE(failed.start()); CHECK_FALSE(failed.cancel());
}

TEST_CASE("cancelled and failed jobs can be retried from zero progress") {
  JobLifecycle cancelled(100);
  REQUIRE(cancelled.start()); REQUIRE(cancelled.progress(40)); REQUIRE(cancelled.cancel());
  REQUIRE(cancelled.retry());
  CHECK(cancelled.snapshot().status == JobStatus::queued);
  CHECK(cancelled.snapshot().frames_processed == 0);
  REQUIRE(cancelled.snapshot().total_frames.has_value());
  CHECK(*cancelled.snapshot().total_frames == 100);
  REQUIRE(cancelled.start()); REQUIRE(cancelled.progress(100)); REQUIRE(cancelled.complete());

  JobLifecycle failed;
  REQUIRE(failed.fail()); REQUIRE(failed.retry());
  CHECK(failed.snapshot().status == JobStatus::queued);
  CHECK_FALSE(failed.snapshot().total_frames.has_value());
  CHECK_FALSE(failed.retry());
}
TEST_CASE("job lifecycle retry uses shared terminal predicate") {
  CHECK(isTerminal(JobStatus::cancelled));
  CHECK(isTerminal(JobStatus::failed));
  CHECK(isTerminal(JobStatus::complete));
  CHECK_FALSE(isTerminal(JobStatus::running));
}
TEST_CASE("shared job contract rejects terminal resurrection and invalid progress") {
  CHECK(canTransition(JobStatus::queued, JobStatus::running));
  CHECK(canTransition(JobStatus::running, JobStatus::complete));
  CHECK(canTransition(JobStatus::running, JobStatus::cancelled));
  CHECK_FALSE(canTransition(JobStatus::complete, JobStatus::running));
  CHECK_FALSE(canTransition(JobStatus::failed, JobStatus::queued));
  CHECK(isValidProgress(JobProgress{0, 0, JobStatus::queued}));
  CHECK(isValidProgress(JobProgress{4, 8, JobStatus::running}));
  CHECK_FALSE(isValidProgress(JobProgress{9, 8, JobStatus::running}));
}
TEST_CASE("known total predicate distinguishes unknown and zero totals") {
  CHECK_FALSE(hasKnownTotal(std::optional<std::uint64_t>{}));
  CHECK(hasKnownTotal(std::optional<std::uint64_t>{0}));
}
TEST_CASE("terminal predicate guards cancellation and failure transitions") {
  JobLifecycle done; REQUIRE(done.start()); REQUIRE(done.complete());
  CHECK_FALSE(done.cancel()); CHECK_FALSE(done.fail());
}

TEST_CASE("commit decision serializes cancellation at the commit boundary") {
 CommitDecision early; CHECK_FALSE(early.begin_commit(true)); CHECK(early.cancelled());
 CHECK_FALSE(early.begin_commit(false)); CHECK_FALSE(early.finish(true));
 CommitDecision late; REQUIRE(late.begin_commit(false)); CHECK_FALSE(late.begin_commit(true));
 REQUIRE(late.finish(true) == PublishOutcome::committed); CHECK_FALSE(late.finish(false));
 CommitDecision failed; REQUIRE(failed.begin_commit(false)); REQUIRE(failed.finish(false) == PublishOutcome::error);
}
TEST_CASE("verification distinguishes stale input, cancellation and storage errors") {
 CHECK_FALSE(verification_failure(FileVerificationOutcome::match));
 CHECK(verification_failure(FileVerificationOutcome::changed) == PublishOutcome::stale_input);
 CHECK(verification_failure(FileVerificationOutcome::unverifiable) == PublishOutcome::stale_input);
 CHECK(verification_failure(FileVerificationOutcome::cancelled) == PublishOutcome::cancelled);
 CHECK(verification_failure(FileVerificationOutcome::error) == PublishOutcome::error);
 CHECK(verification_failure(static_cast<FileVerificationOutcome>(-1)) == PublishOutcome::error);
}
