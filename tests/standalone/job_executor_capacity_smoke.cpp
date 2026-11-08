#include "beatglass/library/detail/job_executor.hpp"
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>
namespace { void require(bool ok, const char* msg) { if (!ok) { std::cerr << msg << '\n'; std::exit(EXIT_FAILURE); } } }
int main() {
  using namespace beatglass::library_detail; JobExecutor executor(1); std::mutex mutex; std::condition_variable started, release; bool running=false, go=false;
  require(executor.submit(0,[&](std::stop_token){ { std::lock_guard lock(mutex); running=true; } started.notify_one(); std::unique_lock lock(mutex); release.wait(lock,[&]{return go;}); }),"active job rejected");
  { std::unique_lock lock(mutex); require(started.wait_for(lock,std::chrono::seconds(1),[&]{return running;}),"active job did not start"); }
  auto queued=executor.submit_tracked(0,[](std::stop_token,const auto&){return TrackedState::completed;}); require(queued!=nullptr,"queued job rejected");
  require(!executor.submit(0,[](std::stop_token){}),"full queue accepted extra work");
  { std::lock_guard lock(mutex); go=true; } release.notify_one();
  for(int i=0;i<100&&queued->state()!=TrackedState::completed;++i) std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(queued->state()==TrackedState::completed,"queued job did not drain");
  require(executor.submit(0,[](std::stop_token){}),"capacity was not released after drain"); executor.shutdown(); return EXIT_SUCCESS;
}
