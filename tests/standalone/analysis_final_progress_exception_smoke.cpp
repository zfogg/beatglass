#include "beatglass/library/analysis_runner.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <thread>
namespace { void require(bool ok, const char* msg) { if (!ok) { std::cerr << msg << '\n'; std::exit(EXIT_FAILURE); } } }
int main() {
  using namespace beatglass::library; using namespace beatglass::library_detail;
  JobExecutor executor(1); AnalysisRunner runner(executor);
  AnalysisOperation op; op.source.path="track.wav"; op.source.size_bytes=12; op.source.content_hash="hash";
  op.identity.content_hash="hash"; op.identity.engine_version="engine"; op.identity.configuration_id="config"; op.identity.mode="offline";
  op.progress = [](std::uint64_t frames, std::uint64_t) { if (frames != 0) throw std::runtime_error("final progress failure"); };
  op.serialize=[] { return std::string("result"); };
  op.verify=[](std::stop_token) { return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match,{}}; };
  auto handle=runner.submit(std::move(op)); require(handle!=nullptr,"submit rejected");
  for(int i=0;i<100&&handle->state()!=TrackedState::failed;++i) std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(handle->state()==TrackedState::failed,"final progress exception completed successfully"); executor.shutdown(); return EXIT_SUCCESS;
}
