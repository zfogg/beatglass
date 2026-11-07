#include "beatglass/capture/detail/pipewire_discovery.hpp"
#include "beatglass/capture/detail/pipewire_monitor.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
int main() {
  using namespace beatglass::pipewire;
  const auto sinks = discover_sinks(std::chrono::seconds(2));
  if (!sinks.error.empty()) { std::cerr << sinks.error << '\n'; return 1; }
  std::string serial;
  for (const auto& sink : sinks.sinks) if (sink.name == "beatglass_test_sink") serial = sink.serial;
  if (serial.empty()) { std::cerr << "virtual sink missing\n"; return 2; }
  Monitor monitor(1);
  if (!monitor.start(serial)) { std::cerr << "start rejected\n"; return 3; }
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(12);
  auto block = std::make_unique<MonitorIngress::Block>();
  bool nonzero = false;
  while (std::chrono::steady_clock::now() < deadline) {
    while (monitor.pop(*block)) {
      for (std::size_t i = 0; i < block->sample_count; ++i) if (std::abs(block->samples[i]) > 0.1f) nonzero = true;
    }
    if (nonzero) break;
    if (monitor.state() == MonitorState::error || monitor.state() == MonitorState::interrupted) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  auto state = monitor.state();
  monitor.stop();
  if (!nonzero) { std::cerr << "no output PCM, state=" << static_cast<unsigned>(state) << " diagnostic=" << monitor.diagnostic_code() << '\n'; return 4; }
  std::cout << "observed virtual sink PCM\n";
}
