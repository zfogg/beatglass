#include "beatglass/library/detail/job_executor.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
namespace { void require(bool ok, const char* msg) { if (!ok) { std::cerr << msg << '\n'; std::exit(EXIT_FAILURE); } } }
int main() {
  using namespace beatglass::library_detail; JobExecutor executor(1);
  auto handle=executor.submit_tracked(0,[](std::stop_token stop,const auto&){ while(!stop.stop_requested()) std::this_thread::yield(); return TrackedState::cancelled; });
  require(handle!=nullptr,"tracked job rejected"); std::this_thread::sleep_for(std::chrono::milliseconds(5));
  std::vector<std::thread> cancellers; for(int i=0;i<8;++i) cancellers.emplace_back([&]{handle->cancel();});
  for(auto& canceller:cancellers) canceller.join();
  for(int i=0;i<100&&handle->state()!=TrackedState::cancelled;++i) std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(handle->state()==TrackedState::cancelled,"concurrent cancellation did not stabilize"); executor.shutdown(); return EXIT_SUCCESS;
}
