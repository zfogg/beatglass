#include "beatglass/library/job_lifecycle.hpp"

#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "lifecycle smoke failure: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}
}

int main() {
  using namespace beatglass;
  JobLifecycle lifecycle(10);
  require(!lifecycle.complete(), "queued job completed");
  require(lifecycle.start(), "queued job did not start");
  require(lifecycle.progress(6), "valid progress was rejected");
  require(!lifecycle.progress(5), "progress regression was accepted");
  require(!lifecycle.progress(11), "progress beyond total was accepted");
  require(!lifecycle.complete(), "incomplete job completed");
  require(lifecycle.progress(10), "final progress was rejected");
  require(lifecycle.complete(), "complete job was rejected");
  require(!lifecycle.cancel(), "terminal cancellation was accepted");

  JobLifecycle retryable;
  require(retryable.start(), "retryable job did not start");
  require(retryable.progress(4), "retryable progress was rejected");
  require(retryable.cancel(), "running cancellation was rejected");
  require(retryable.retry(), "cancelled job did not requeue");
  require(retryable.snapshot().frames_processed == 0, "retry retained partial progress");
  require(retryable.start() && retryable.fail(), "requeued failure path failed");
  require(retryable.retry(), "failed job did not requeue");
  return EXIT_SUCCESS;
}
