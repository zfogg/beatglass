#include "beatglass/library/analysis_runner.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "batch smoke failure: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}

beatglass::library::AnalysisOperation operation(int id) {
  beatglass::library::AnalysisOperation op;
  op.source.path = "track-" + std::to_string(id) + ".wav";
  op.source.content_hash = "hash-" + std::to_string(id);
  op.identity.content_hash = op.source.content_hash;
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
  JobExecutor executor(3);
  AnalysisRunner runner(executor);
  require(runner.submit_batch({}).empty(), "empty batch was not empty");
  auto handles = runner.submit_batch({operation(1), operation(2), operation(3)});
  require(handles.size() == 3, "capacity-sized batch was not accepted");
  for (const auto& handle : handles) {
    for (int i = 0; i != 100 && handle->state() != TrackedState::completed; ++i)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    require(handle->state() == TrackedState::completed,
            "batch operation did not complete");
  }
  executor.shutdown();
  return EXIT_SUCCESS;
}
