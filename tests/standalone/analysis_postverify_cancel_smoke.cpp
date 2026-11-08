#include "beatglass/library/analysis_runner.hpp"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
namespace { void require(bool ok, const char* msg) { if (!ok) { std::cerr << msg << '\n'; std::exit(EXIT_FAILURE); } } }
int main() {
  using namespace beatglass::library; using namespace beatglass::library_detail;
  JobExecutor executor(1); AnalysisRunner runner(executor);
  std::atomic_bool verifying=false, release=false, serialized=false;
  AnalysisOperation op; op.source.path="track.wav"; op.source.content_hash="hash";
  op.identity.content_hash="hash"; op.identity.engine_version="engine";
  op.identity.configuration_id="config"; op.identity.mode="offline";
  op.verify = [&](std::stop_token) { verifying=true; while(!release.load()) std::this_thread::yield(); return beatglass::FileVerificationResult{beatglass::FileVerificationOutcome::match,{}}; };
  op.serialize = [&] { serialized=true; return std::string("result"); };
  auto handle=runner.submit(std::move(op)); require(handle!=nullptr,"submit rejected");
  for(int i=0;i<100&&!verifying.load();++i) std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(verifying.load(),"verifier did not start"); handle->cancel(); release=true;
  for(int i=0;i<100&&handle->state()==TrackedState::running;++i) std::this_thread::sleep_for(std::chrono::milliseconds(1));
  require(handle->state()==TrackedState::cancelled,"post-verification cancellation completed");
  require(!serialized,"post-verification cancellation serialized"); executor.shutdown(); return EXIT_SUCCESS;
}
