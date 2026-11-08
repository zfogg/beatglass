#include "beatglass/library/detail/job_executor.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "executor exception smoke failure: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}
}

int main() {
  using namespace beatglass::library_detail;
  JobExecutor executor(2);
  require(executor.submit(0, [](std::stop_token) { throw std::runtime_error("expected"); }),
          "throwing work was rejected");
  auto handle = executor.submit_tracked(0, [](std::stop_token, const auto&) {
    return TrackedState::completed;
  });
  require(handle != nullptr, "follow-up tracked work was rejected");
  for (int i = 0; i != 100 && handle->state() != TrackedState::completed; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(handle->state() == TrackedState::completed,
          "worker did not survive untracked exception");
  executor.shutdown();
  return EXIT_SUCCESS;
}
