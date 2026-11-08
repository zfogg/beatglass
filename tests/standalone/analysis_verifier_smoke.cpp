#include "beatglass/library/analysis_runner.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "verifier smoke failure: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}

beatglass::library::AnalysisOperation operation(beatglass::FileVerificationOutcome outcome,
                                                bool& serialized) {
  beatglass::library::AnalysisOperation op;
  op.source.path = "track.wav";
  op.source.content_hash = "hash";
  op.identity.content_hash = "hash";
  op.identity.engine_version = "engine";
  op.identity.configuration_id = "config";
  op.identity.mode = "offline";
  op.serialize = [&serialized] { serialized = true; return std::string("result"); };
  op.verify = [outcome](std::stop_token) {
    return beatglass::FileVerificationResult{outcome, {}};
  };
  return op;
}
}

int main() {
  using namespace beatglass::library;
  using namespace beatglass::library_detail;
  JobExecutor executor(2);
  AnalysisRunner runner(executor);
  bool serialized = false;
  auto changed = runner.submit(operation(beatglass::FileVerificationOutcome::changed, serialized));
  require(changed != nullptr, "changed-input operation was rejected");
  for (int i = 0; i != 100 && changed->state() != TrackedState::failed; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(changed->state() == TrackedState::failed, "changed input did not fail closed");
  require(!serialized, "changed input was serialized");

  serialized = false;
  auto cancelled = runner.submit(operation(beatglass::FileVerificationOutcome::cancelled, serialized));
  require(cancelled != nullptr, "cancelled-input operation was rejected");
  for (int i = 0; i != 100 && cancelled->state() != TrackedState::cancelled; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(cancelled->state() == TrackedState::cancelled, "cancelled verification was not cancelled");
  require(!serialized, "cancelled input was serialized");
  executor.shutdown();
  return EXIT_SUCCESS;
}
