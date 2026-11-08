#include "beatglass/library/analysis_runner.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "empty-result smoke failure: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}
}

int main() {
  using namespace beatglass::library;
  using namespace beatglass::library_detail;
  JobExecutor executor(1);
  AnalysisRunner runner(executor);
  AnalysisOperation operation;
  operation.source.path = "track.wav";
  operation.source.content_hash = "hash";
  operation.identity.content_hash = "hash";
  operation.identity.engine_version = "engine";
  operation.identity.configuration_id = "config";
  operation.identity.mode = "offline";
  operation.serialize = [] { return std::string{}; };
  operation.verify = [](std::stop_token) {
    return beatglass::FileVerificationResult{
        beatglass::FileVerificationOutcome::match, {}};
  };
  auto handle = runner.submit(std::move(operation));
  require(handle != nullptr, "empty-result operation was rejected");
  for (int i = 0; i != 100 && handle->state() == TrackedState::queued; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  for (int i = 0; i != 100 && handle->state() == TrackedState::running; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(handle->state() == TrackedState::failed,
          "empty serialization was reported as successful");
  executor.shutdown();
  return EXIT_SUCCESS;
}
