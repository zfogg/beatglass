#include "beatglass/library/detail/job_executor.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
namespace { void require(bool ok, const char* msg) { if (!ok) { std::cerr << msg << '\n'; std::exit(EXIT_FAILURE); } } }
int main() {
  using namespace beatglass::library_detail; JobExecutor executor(1);
  auto handle=executor.submit_tracked(0,[](std::stop_token,const auto& progress){ progress(7); return TrackedState::completed; });
  require(handle!=nullptr,"tracked job rejected");
  for(int i=0;i<100&&handle->state()!=TrackedState::completed;++i) std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(handle->state()==TrackedState::completed,"job did not complete"); handle->cancel();
  require(handle->state()==TrackedState::completed&&handle->frames()==7,"terminal cancel rewrote handle"); executor.shutdown(); return EXIT_SUCCESS;
}
