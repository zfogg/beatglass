#include "beatglass/library/detail/job_executor.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "executor smoke failure: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}
}

int main() {
  using namespace beatglass::library_detail;
  JobExecutor empty(0);
  require(!empty.submit(0, [](std::stop_token) {}), "zero-capacity submit accepted");
  require(!empty.submit_tracked(0, [](std::stop_token, const auto&) {
    return TrackedState::completed;
  }), "zero-capacity tracked submit accepted");
  empty.shutdown();

  JobExecutor executor(1);
  auto handle = executor.submit_tracked(3, [](std::stop_token stop, const auto& progress) {
    progress(1);
    if (stop.stop_requested()) return TrackedState::cancelled;
    return TrackedState::completed;
  });
  require(handle != nullptr, "tracked submit rejected unexpectedly");
  for (int i = 0; i != 100 && handle->state() == TrackedState::queued; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  for (int i = 0; i != 100 && handle->state() == TrackedState::running; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(handle->state() == TrackedState::completed, "tracked job did not complete");
  require(handle->frames() == 1, "tracked progress was not retained");
  executor.shutdown();
  executor.shutdown();
  return EXIT_SUCCESS;
}
