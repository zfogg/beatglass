#include "beatglass/library/analysis_runner.hpp"

#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "capacity smoke failure: " << message << '\n';
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
  JobExecutor executor(0);
  AnalysisRunner runner(executor);
  require(!runner.submit(operation()), "zero-capacity submit was accepted");
  auto retryable = runner.submit_retryable(operation());
  require(retryable != nullptr, "retryable controller was not returned");
  require(retryable->state() == TrackedState::failed,
          "rejected retryable submission did not fail closed");
  require(!retryable->retry(), "retry was accepted without queue capacity");
  executor.shutdown();
  return EXIT_SUCCESS;
}
