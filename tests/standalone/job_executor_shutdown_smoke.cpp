#include "beatglass/library/detail/job_executor.hpp"
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>
namespace { void require(bool ok, const char* msg) { if (!ok) { std::cerr << msg << '\n'; std::exit(EXIT_FAILURE); } } }
int main() {
  using namespace beatglass::library_detail;
  JobExecutor executor(1); std::mutex mutex; std::condition_variable started; bool running=false;
  auto handle=executor.submit_tracked(0,[&](std::stop_token stop,const auto&) {
    { std::lock_guard lock(mutex); running=true; } started.notify_one();
    while(!stop.stop_requested()) std::this_thread::yield();
    return TrackedState::cancelled;
  });
  require(handle!=nullptr,"active tracked job was rejected");
  { std::unique_lock lock(mutex); require(started.wait_for(lock,std::chrono::seconds(1),[&]{return running;}),"job did not start"); }
  executor.shutdown();
  require(handle->state()==TrackedState::cancelled,"shutdown did not cancel active work");
  return EXIT_SUCCESS;
}
