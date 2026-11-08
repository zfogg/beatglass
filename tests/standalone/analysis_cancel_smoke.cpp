#include "beatglass/library/analysis_runner.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "analysis cancellation smoke failure: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}
}

int main() {
  using namespace beatglass::library;
  using namespace beatglass::library_detail;
  JobExecutor executor(1);
  AnalysisRunner runner(executor);
  std::atomic_bool verified = false;
  AnalysisOperation operation;
  operation.source.path = "track.wav";
  operation.source.content_hash = "hash";
  operation.identity.content_hash = "hash";
  operation.identity.engine_version = "engine";
  operation.identity.configuration_id = "config";
  operation.identity.mode = "offline";
  operation.serialize = [] { return std::string("result"); };
  operation.verify = [&](std::stop_token stop) {
    while (!stop.stop_requested()) std::this_thread::yield();
    verified = true;
    return beatglass::FileVerificationResult{
        beatglass::FileVerificationOutcome::cancelled, {}};
  };
  auto handle = runner.submit(std::move(operation));
  require(handle != nullptr, "cancellable operation was rejected");
  for (int i = 0; i != 100 && handle->state() == TrackedState::queued; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  handle->cancel();
  for (int i = 0; i != 100 && handle->state() != TrackedState::cancelled; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(handle->state() == TrackedState::cancelled,
          "analysis cancellation did not reach terminal state");
  require(verified, "verifier did not observe cancellation");
  executor.shutdown();
  return EXIT_SUCCESS;
}
