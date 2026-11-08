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
    std::cerr << "queued cancellation smoke failure: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}
}

int main() {
  using namespace beatglass::library_detail;
  JobExecutor executor(1);
  std::mutex mutex;
  std::condition_variable started;
  std::condition_variable release;
  bool running = false;
  bool go = false;
  bool queued_ran = false;
  require(executor.submit(0, [&](std::stop_token) {
    {
      std::lock_guard lock(mutex);
      running = true;
    }
    started.notify_one();
    std::unique_lock lock(mutex);
    release.wait(lock, [&] { return go; });
  }), "active job was rejected");
  {
    std::unique_lock lock(mutex);
    require(started.wait_for(lock, std::chrono::seconds(1), [&] { return running; }),
            "active job did not start");
  }
  auto queued = executor.submit_tracked(0, [&](std::stop_token, const auto&) {
    std::lock_guard lock(mutex);
    queued_ran = true;
    return TrackedState::completed;
  });
  require(queued != nullptr, "queued tracked job was rejected");
  queued->cancel();
  require(queued->state() == TrackedState::cancelled,
          "queued cancellation was not immediate");
  {
    std::lock_guard lock(mutex);
    go = true;
  }
  release.notify_one();
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  require(!queued_ran, "cancelled queued work executed");
  executor.shutdown();
  return EXIT_SUCCESS;
}
