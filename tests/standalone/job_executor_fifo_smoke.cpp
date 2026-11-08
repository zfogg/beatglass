#include "beatglass/library/detail/job_executor.hpp"
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
namespace { void require(bool ok, const char* msg) { if (!ok) { std::cerr << msg << '\n'; std::exit(EXIT_FAILURE); } } }
int main() {
  using namespace beatglass::library_detail; JobExecutor executor(2); std::mutex mutex; std::condition_variable started, release, done; bool running=false, go=false; std::vector<int> order;
  require(executor.submit(0,[&](std::stop_token){ { std::lock_guard lock(mutex); running=true; } started.notify_one(); std::unique_lock lock(mutex); release.wait(lock,[&]{return go;}); order.push_back(0); done.notify_one(); }),"active job rejected");
  { std::unique_lock lock(mutex); require(started.wait_for(lock,std::chrono::seconds(1),[&]{return running;}),"active job did not start"); }
  require(executor.submit(1,[&](std::stop_token){ std::lock_guard lock(mutex); order.push_back(1); done.notify_one(); }),"first queued job rejected");
  require(executor.submit(1,[&](std::stop_token){ std::lock_guard lock(mutex); order.push_back(2); done.notify_one(); }),"second queued job rejected");
  { std::lock_guard lock(mutex); go=true; } release.notify_one();
  { std::unique_lock lock(mutex); require(done.wait_for(lock,std::chrono::seconds(1),[&]{return order.size()==3;}),"jobs did not complete"); }
  executor.shutdown(); require(order[0]==0&&order[1]==1&&order[2]==2,"equal-priority FIFO ordering failed"); return EXIT_SUCCESS;
}
