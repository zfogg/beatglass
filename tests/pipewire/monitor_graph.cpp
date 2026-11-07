#include "beatglass/capture/detail/pipewire_discovery.hpp"
#include "beatglass/capture/detail/pipewire_monitor.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
int main(int argc, char**) {
  const bool missing = argc > 1;
  using namespace beatglass::pipewire;
  const auto sinks = discover_sinks(std::chrono::seconds(2));
  if (!sinks.error.empty()) { std::cerr << sinks.error << '\n'; return 1; }
  Sink selected;
  for (const auto& sink : sinks.sinks) if (sink.name == "beatglass_test_sink") selected = sink;
  if (selected.serial.empty()) { std::cerr << "virtual sink missing\n"; return 2; }
  if (missing) selected = {"999999999999", "beatglass_missing_sink", ""};
  Monitor monitor(1);
  if (!monitor.start(selected)) { if (missing) { std::cout << "missing target rejected\n"; return 0; } std::cerr << "start rejected\n"; return 3; }
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(missing ? 5 : 12);
  auto block = std::make_unique<MonitorIngress::Block>();
  bool nonzero = false;
  unsigned positive_crossings = 0;
  float previous = 0.0f;
  std::uint64_t last_end = 0;
  while (std::chrono::steady_clock::now() < deadline) {
    while (monitor.pop(*block)) {
      if (block->epoch != 1 || block->format.sample_rate != 48000 || block->format.channels != 2 ||
          block->first_frame < last_end || block->sample_count % 2) return 6;
      last_end = block->first_frame + block->sample_count / 2;
      for (std::size_t i = 0; i < block->sample_count; i += 2) {
        const float sample = block->samples[i];
        if (std::abs(sample) > 0.1f) nonzero = true;
        if (previous <= 0.0f && sample > 0.0f) ++positive_crossings;
        previous = sample;
      }
    }
    if (nonzero && positive_crossings >= 20) break;
    if (monitor.state() == MonitorState::error || monitor.state() == MonitorState::interrupted) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  auto state = monitor.state();
  auto diagnostic = monitor.diagnostic_code();
  monitor.stop();
  if (missing) {
    if (nonzero || state == MonitorState::streaming) { std::cerr << "missing target fell back to another sink\n"; return 5; }
    std::cout << "missing target did not fall back\n"; return 0;
  }
  if (!nonzero || positive_crossings < 20) { std::cerr << "no output PCM, state=" << static_cast<unsigned>(state) << " diagnostic=" << diagnostic << '\n'; return 4; }
  std::cout << "observed virtual sink PCM\n";
}
