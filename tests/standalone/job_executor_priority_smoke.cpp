#include "beatglass/library/detail/job_executor.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <vector>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "executor priority smoke failure: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}
}

int main() {
  using namespace beatglass::library_detail;
  JobExecutor executor(2);
  std::mutex mutex;
  std::condition_variable ready;
  std::condition_variable release;
  std::condition_variable done;
  bool started = false;
  bool go = false;
  std::vector<int> order;

  require(executor.submit(0, [&](std::stop_token) {
    {
      std::lock_guard lock(mutex);
      started = true;
    }
    ready.notify_one();
    std::unique_lock lock(mutex);
    release.wait(lock, [&] { return go; });
    order.push_back(0);
    done.notify_one();
  }), "active job was rejected");
  {
    std::unique_lock lock(mutex);
    require(ready.wait_for(lock, std::chrono::seconds(1), [&] { return started; }),
            "active job did not start");
  }
  require(executor.submit(1, [&](std::stop_token) {
    std::lock_guard lock(mutex);
    order.push_back(1);
    done.notify_one();
  }),
          "low-priority job was rejected");
  require(executor.submit(10, [&](std::stop_token) {
    std::lock_guard lock(mutex);
    order.push_back(10);
    done.notify_one();
  }),
          "high-priority job was rejected");
  {
    std::lock_guard lock(mutex);
    go = true;
  }
  release.notify_one();
  {
    std::unique_lock lock(mutex);
    require(done.wait_for(lock, std::chrono::seconds(1), [&] { return order.size() == 3; }),
            "queued jobs did not complete");
  }
  executor.shutdown();
  require(order.size() == 3, "not all jobs completed");
  require(order[0] == 0 && order[1] == 10 && order[2] == 1,
          "priority ordering was not honored");
  return EXIT_SUCCESS;
}
