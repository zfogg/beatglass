#include "beatglass/capture/coreaudio_source_internal.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <thread>
using namespace beatglass;
using namespace beatglass::coreaudio;

int main(int argc, char** argv) {
  if (argc != 2) { std::cerr << "Pass the selected output UID; play audio continuously through it from another application.\n"; return 2; }
  SourceEpochAllocator epochs;
  SourceAdapter source(epochs, make_native_backend());
  std::uint64_t previous_epoch = 0;
  for (int cycle = 0; cycle != 2; ++cycle) {
    std::uint64_t frames = 0, nonzero = 0, gaps = 0;
    const auto start = source.start(argv[1], [&](const AudioBlock& block) {
      frames += block.samples.size() / block.format.channels;
      gaps += block.discontinuity;
      for (const auto value : block.samples) nonzero += std::abs(value) > 0.00001f;
    });
    if (!start.attempt) { std::cerr << "start failed: " << start.diagnostic << '\n'; return 1; }
    const auto event = source.take_event();
    if (!event || event->state != SourceState::running || !event->epoch || !event->format ||
        *event->epoch <= previous_epoch || !source.acknowledge_running(*start.attempt, event->sequence)) {
      std::cerr << "invalid running event or reused epoch\n"; return 1;
    }
    previous_epoch = *event->epoch;
    const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(6);
    while (std::chrono::steady_clock::now() < end) {
      if (!source.pump()) {
        const auto change = source.take_event();
        if (change) { std::cerr << "capture interrupted: " << change->status.message << '\n'; return 1; }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
    }
    const bool stopped = source.stop();
    std::cout << "cycle=" << cycle << " epoch=" << previous_epoch << " frames=" << frames
              << " nonzero_samples=" << nonzero << " discontinuities=" << gaps << " stopped=" << stopped << '\n';
    if (!stopped || !frames || !nonzero) return 1;
  }
  return 0;
}
