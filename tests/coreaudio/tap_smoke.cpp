#include "beatglass/capture/coreaudio_internal.hpp"
#include "beatglass/capture/coreaudio_tap_internal.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char** argv) {
  std::string error;
  const auto outputs = beatglass::coreaudio::enumerate_outputs(error);
  if (!error.empty()) { std::cerr << error << '\n'; return 1; }
  if (argc == 1) {
    for (const auto& output : outputs) std::cout << output.id << '\t' << output.name << '\n';
    std::cerr << "Pass the exact output UID as the sole argument, then play audio through that output from another application.\n";
    return 0;
  }
  if (argc != 2) return 2;
  beatglass::coreaudio::OutputTap tap;
  if (!tap.start(argv[1], 1)) { std::cerr << tap.status().message << '\n'; return 1; }
  beatglass::coreaudio::CaptureBlock block;
  std::uint64_t frames = 0, nonzero = 0, gaps = 0;
  double expected_sample_time = 0;
  bool have_sample_time = false;
  const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(15);
  while (std::chrono::steady_clock::now() < end) {
    if (tap.pop(block)) {
      frames += block.sample_count / block.channels;
      gaps += block.discontinuity;
      if (block.discontinuity) {
        std::cout << "gap first_frame=" << block.first_frame;
        if (block.source_sample_time_valid && have_sample_time)
          std::cout << " native_sample_delta=" << (block.source_sample_time - expected_sample_time);
        else std::cout << " native_sample_delta=unknown";
        std::cout << '\n';
      }
      have_sample_time = block.source_sample_time_valid;
      if (have_sample_time) expected_sample_time = block.source_sample_time + block.sample_count / block.channels;
      for (std::size_t i = 0; i < block.sample_count; ++i) nonzero += std::abs(block.samples[i]) > 0.00001f;
    } else {
      const auto state = tap.status();
      if (state.state != beatglass::coreaudio::TapState::running) { std::cerr << state.message << '\n'; return 1; }
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  }
  std::cout << "frames=" << frames << " nonzero_samples=" << nonzero << " discontinuities=" << gaps
            << " dropped_frames=" << tap.dropped_frames() << " timestamp_valid=" << (have_sample_time ? 1 : 0) << '\n';
  tap.stop();
  return frames && nonzero && have_sample_time ? 0 : 1;
}
