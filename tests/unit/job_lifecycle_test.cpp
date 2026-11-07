#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/job_lifecycle.hpp"
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
