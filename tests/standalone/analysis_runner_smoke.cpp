#include "beatglass/library/analysis_runner.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "runner smoke failure: " << message << '\n';
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
  JobExecutor executor(2);
  AnalysisRunner runner(executor);

  auto handle = runner.submit(operation());
  require(handle != nullptr, "valid operation was rejected");
  for (int i = 0; i != 100 && handle->state() != TrackedState::completed; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(handle->state() == TrackedState::completed, "valid operation did not complete");

  auto invalid = runner.submit(AnalysisOperation{});
  require(invalid != nullptr, "invalid operation was not queued");
  for (int i = 0; i != 100 && invalid->state() != TrackedState::failed; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(invalid->state() == TrackedState::failed, "invalid operation did not fail closed");

  auto retryable = runner.submit_retryable(AnalysisOperation{});
  require(retryable != nullptr, "retryable controller was not created");
  for (int i = 0; i != 100 && retryable->state() != TrackedState::failed; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(retryable->state() == TrackedState::failed,
          "retryable invalid operation did not retain failed state");
  require(retryable->retry(), "retryable controller rejected a fresh retry");
  executor.shutdown();
  return EXIT_SUCCESS;
}
