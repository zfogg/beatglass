#include "beatglass/library/detail/job_executor.hpp"
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
namespace { void require(bool ok, const char* msg) { if (!ok) { std::cerr << msg << '\n'; std::exit(EXIT_FAILURE); } } }
int main() {
  using namespace beatglass::library_detail; JobExecutor executor(4); std::atomic_int accepted=0, completed=0;
  std::vector<std::thread> producers;
  for(int p=0;p<4;++p) producers.emplace_back([&] { for(int i=0;i<25;++i) if(executor.submit(0,[&](std::stop_token){ ++completed; })) ++accepted; });
  for(auto& producer:producers) producer.join();
  for(int i=0;i<100&&completed.load()!=accepted.load();++i) std::this_thread::sleep_for(std::chrono::milliseconds(1));
  executor.shutdown();
  require(accepted.load()<=100,"executor admitted more work than submitted"); require(completed.load()==accepted.load(),"accepted work was lost before shutdown"); return EXIT_SUCCESS;
}
