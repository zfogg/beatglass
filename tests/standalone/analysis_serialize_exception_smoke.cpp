#include "beatglass/library/analysis_runner.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "serialization exception smoke failure: " << message << '\n';
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
  operation.serialize = []() -> std::string {
    throw std::runtime_error("serialization failure");
  };
  operation.verify = [](std::stop_token) {
    return beatglass::FileVerificationResult{
        beatglass::FileVerificationOutcome::match, {}};
  };
  auto handle = runner.submit(std::move(operation));
  require(handle != nullptr, "throwing serialization was rejected");
  for (int i = 0; i != 100 && handle->state() != TrackedState::failed; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(handle->state() == TrackedState::failed,
          "serialization exception did not fail closed");
  executor.shutdown();
  return EXIT_SUCCESS;
}
