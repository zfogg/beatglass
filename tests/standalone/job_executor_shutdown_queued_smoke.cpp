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
  require(executor.submit(0,[&](std::stop_token stop) { { std::lock_guard lock(mutex); running=true; } started.notify_one(); while(!stop.stop_requested()) std::this_thread::yield(); }),"active job rejected");
  { std::unique_lock lock(mutex); require(started.wait_for(lock,std::chrono::seconds(1),[&]{return running;}),"active job did not start"); }
  auto queued=executor.submit_tracked(0,[](std::stop_token,const auto&) { return TrackedState::completed; });
  require(queued!=nullptr,"queued tracked job rejected"); executor.shutdown();
  require(queued->state()==TrackedState::cancelled,"shutdown did not cancel queued work"); return EXIT_SUCCESS;
}
