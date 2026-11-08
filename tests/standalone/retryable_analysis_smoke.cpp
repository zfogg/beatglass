#include "beatglass/library/analysis_runner.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "retryable analysis smoke failure: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}

beatglass::library::AnalysisOperation operation() {
  beatglass::library::AnalysisOperation op;
  op.source.path = "track.wav";
  op.source.content_hash = "hash";
  op.identity.content_hash = "hash";
  op.identity.engine_version = "engine";
  op.identity.configuration_id = "config";
  op.identity.mode = "offline";
  op.serialize = [] { return std::string("result"); };
  op.verify = [](std::stop_token) {
    return beatglass::FileVerificationResult{
        beatglass::FileVerificationOutcome::match, {}};
  };
  return op;
}
}

int main() {
  using namespace beatglass::library;
  using namespace beatglass::library_detail;
  JobExecutor executor(1);
  AnalysisRunner runner(executor);
  auto retryable = runner.submit_retryable(operation());
  require(retryable != nullptr, "retryable controller was not created");
  require(!retryable->retry(), "duplicate retry was accepted while active");
  for (int i = 0; i != 100 && retryable->state() != TrackedState::completed; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(retryable->state() == TrackedState::completed,
          "first retryable attempt did not complete");
  require(retryable->retry(), "completed attempt could not be retried");
  for (int i = 0; i != 100 && retryable->state() != TrackedState::completed; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(retryable->state() == TrackedState::completed,
          "retried attempt did not complete");
  executor.shutdown();
  return EXIT_SUCCESS;
}
