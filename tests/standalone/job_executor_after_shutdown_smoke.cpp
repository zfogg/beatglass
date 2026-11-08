#include "beatglass/library/detail/job_executor.hpp"
#include <cstdlib>
#include <iostream>
namespace { void require(bool ok, const char* msg) { if (!ok) { std::cerr << msg << '\n'; std::exit(EXIT_FAILURE); } } }
int main() {
  using namespace beatglass::library_detail; JobExecutor executor(1); executor.shutdown();
  require(!executor.submit(0,[](std::stop_token){}),"untracked submission accepted after shutdown");
  require(!executor.submit_tracked(0,[](std::stop_token,const auto&){return TrackedState::completed;}),"tracked submission accepted after shutdown");
  executor.shutdown(); return EXIT_SUCCESS;
}
