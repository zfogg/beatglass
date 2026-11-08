#include "beatglass/library/detail/job_executor.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "executor cancellation smoke failure: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}
}

int main() {
  using namespace beatglass::library_detail;
  JobExecutor executor(1);
  std::mutex mutex;
  std::condition_variable started;
  bool running = false;
  auto handle = executor.submit_tracked(0, [&](std::stop_token stop, const auto&) {
    {
      std::lock_guard lock(mutex);
      running = true;
    }
    started.notify_one();
    while (!stop.stop_requested()) std::this_thread::yield();
    return TrackedState::cancelled;
  });
  require(handle != nullptr, "tracked cancellation job was rejected");
  {
    std::unique_lock lock(mutex);
    require(started.wait_for(lock, std::chrono::seconds(1), [&] { return running; }),
            "tracked job did not start");
  }
  handle->cancel();
  for (int i = 0; i != 100 && handle->state() != TrackedState::cancelled; ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(handle->state() == TrackedState::cancelled,
          "running cancellation did not reach terminal state");
  executor.shutdown();
  return EXIT_SUCCESS;
}
