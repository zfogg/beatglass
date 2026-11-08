#include "beatglass/library/detail/job_executor.hpp"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
namespace { void require(bool ok, const char* msg) { if (!ok) { std::cerr << msg << '\n'; std::exit(EXIT_FAILURE); } } }
int main() {
  using namespace beatglass::library_detail; JobExecutor executor(1);
  auto handle=executor.submit_tracked(0,[](std::stop_token stop,const auto& progress){ for(std::uint64_t i=1;i<=100&&!stop.stop_requested();++i){ progress(i); std::this_thread::yield(); } return TrackedState::completed; });
  require(handle!=nullptr,"tracked job rejected"); std::atomic_bool bad=false; std::vector<std::thread> readers;
  for(int i=0;i<4;++i) readers.emplace_back([&]{ std::uint64_t previous=0; while(handle->state()!=TrackedState::completed){ auto frames=handle->frames(); if(frames<previous) bad=true; previous=frames; } });
  for(auto& reader:readers) reader.join();
  require(handle->state()==TrackedState::completed&&handle->frames()==100,"readers observed invalid terminal state");
  require(!bad.load(),"frames regressed for a concurrent reader"); executor.shutdown(); return EXIT_SUCCESS;
}
