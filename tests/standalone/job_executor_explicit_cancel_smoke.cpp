#include "beatglass/library/detail/job_executor.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
namespace { void require(bool ok, const char* msg) { if (!ok) { std::cerr << msg << '\n'; std::exit(EXIT_FAILURE); } } }
int main() {
  using namespace beatglass::library_detail; JobExecutor executor(1);
  auto handle=executor.submit_tracked(0,[](std::stop_token,const auto&){ return TrackedState::cancelled; });
  require(handle!=nullptr,"explicit cancellation job rejected");
  for(int i=0;i<100&&handle->state()!=TrackedState::cancelled;++i) std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(handle->state()==TrackedState::cancelled,"explicit cancelled outcome was rewritten"); executor.shutdown(); return EXIT_SUCCESS;
}
