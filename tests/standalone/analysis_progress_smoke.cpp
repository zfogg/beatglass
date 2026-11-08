#include "beatglass/library/analysis_runner.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "progress smoke failure: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}
}

int main() {
  using namespace beatglass::library;
  using namespace beatglass::library_detail;
  JobExecutor executor(1);
  AnalysisRunner runner(executor);
  std::mutex mutex;
  std::uint64_t observed = 0;
  std::uint64_t total = 0;
  AnalysisOperation operation;
  operation.source.path = "track.wav";
  operation.source.size_bytes = 1234;
  operation.source.content_hash = "hash";
  operation.identity.content_hash = "hash";
  operation.identity.engine_version = "engine";
  operation.identity.configuration_id = "config";
  operation.identity.mode = "offline";
  operation.progress = [&](std::uint64_t frames, std::uint64_t maximum) {
    std::lock_guard lock(mutex);
    observed = frames;
    total = maximum;
  };
  operation.serialize = [] { return std::string("result"); };
  operation.verify = [](std::stop_token) {
    return beatglass::FileVerificationResult{
        beatglass::FileVerificationOutcome::match, {}};
  };
  auto handle = runner.submit(std::move(operation));
  require(handle != nullptr, "progress operation was rejected");
  for (int i = 0; i != 100 && handle->state() != TrackedState::completed; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(handle->state() == TrackedState::completed, "progress operation failed");
  {
    std::lock_guard lock(mutex);
    require(observed == 1234 && total == 1234, "final operation progress was not propagated");
  }
  require(handle->frames() == 1234, "tracked final progress was not propagated");
  executor.shutdown();
  return EXIT_SUCCESS;
}
