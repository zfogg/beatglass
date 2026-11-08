#include "beatglass/library/analysis_runner.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "verifier error smoke failure: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}
}

int main() {
  using namespace beatglass::library;
  using namespace beatglass::library_detail;
  JobExecutor executor(1);
  AnalysisRunner runner(executor);
  bool serialized = false;
  AnalysisOperation operation;
  operation.source.path = "track.wav";
  operation.source.content_hash = "hash";
  operation.identity.content_hash = "hash";
  operation.identity.engine_version = "engine";
  operation.identity.configuration_id = "config";
  operation.identity.mode = "offline";
  operation.serialize = [&serialized] { serialized = true; return std::string("result"); };
  operation.verify = [](std::stop_token) {
    return beatglass::FileVerificationResult{
        beatglass::FileVerificationOutcome::error, "verification failed"};
  };
  auto handle = runner.submit(std::move(operation));
  require(handle != nullptr, "verification-error operation was rejected");
  for (int i = 0; i != 100 && handle->state() != TrackedState::failed; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(handle->state() == TrackedState::failed, "verification error did not fail closed");
  require(!serialized, "verification error was serialized");
  executor.shutdown();
  return EXIT_SUCCESS;
}
