#include "beatglass/capture/detail/pipewire_discovery.hpp"
#include "beatglass/capture/source_lifecycle.hpp"
#include "beatglass/capture/detail/pipewire_monitor.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string_view>
#include <thread>
int main(int argc, char** argv) {
  const bool missing = argc > 1 && std::string_view(argv[1]) == "missing";
  using namespace beatglass::pipewire;
  const auto sinks = discover_sinks(std::chrono::seconds(2));
  if (!sinks.error.empty()) { std::cerr << sinks.error << '\n'; return 1; }
  Sink selected;
  for (const auto& sink : sinks.sinks) if (sink.name == "beatglass_test_sink") selected = sink;
  if (selected.serial.empty() && !missing) { std::cerr << "virtual sink missing\n"; return 2; }
  if (missing) selected = {"999999999999", "beatglass_missing_sink", ""};
  beatglass::SourceEpochAllocator allocator;
  const auto epoch = allocator.next();
  beatglass::SourceLifecycle lifecycle;
  const auto attempt = lifecycle.start();
  if (!epoch || !attempt) return 10;
  Monitor monitor(*epoch, *attempt);
  if (!monitor.start(selected).attempt) { if (missing) { std::cout << "missing target rejected\n"; return 0; } std::cerr << "start rejected\n"; return 3; }
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(missing ? 5 : 12);
  auto block = std::make_unique<MonitorIngress::Block>();
  MonitorIngress::Block before_install;
  if (monitor.pop(before_install)) { std::cerr << "PCM escaped before worker installation\n"; return 11; }
  MonitorEvent running;
  const auto start_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
  while (std::chrono::steady_clock::now() < start_deadline && !monitor.pop_event(running))
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  if (monitor.pop(before_install)) { std::cerr << "PCM escaped after running but before worker installation\n"; return 14; }
  if (running.state != beatglass::SourceState::running || running.sequence != 1 || running.epoch != epoch ||
      running.attempt.generation != attempt->generation || running.anchor || running.delivery_lost ||
      lifecycle.notify(running.attempt, running.sequence, running.state, running.epoch) != beatglass::SourceEventResult::accepted ||
      monitor.install_worker_epoch(*epoch + 1, {48000, 2, beatglass::ChannelLayout::stereo}) ||
      monitor.install_worker_epoch(*epoch, {44100, 2, beatglass::ChannelLayout::stereo}) ||
      !monitor.install_worker_epoch(*epoch, {48000, 2, beatglass::ChannelLayout::stereo})) {
    std::cerr << "worker barrier rejected running event or accepted stale format\n"; return 12;
  }
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
  if (!missing && nonzero && positive_crossings >= 20 && argc > 1) {
    MonitorEvent event;
    const auto clock = monitor.graph_clock();
    if (!clock || clock->now_ns <= 0 || !clock->rate_num || !clock->rate_denom) {
      std::cerr << "missing graph clock measurement\n"; return 13;
    }
    { std::ofstream ready(argv[1]); ready << "ready"; }
    const auto loss_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < loss_deadline && monitor.state() == MonitorState::streaming)
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (!monitor.pop_event(event) || event.attempt.generation != attempt->generation || event.sequence != 2 || event.epoch ||
        (event.state != beatglass::SourceState::interrupted && event.state != beatglass::SourceState::error) || event.delivery_lost ||
        lifecycle.notify(event.attempt, event.sequence, event.state, event.epoch) != beatglass::SourceEventResult::accepted) {
      std::cerr << "missing ordered target-loss event state=" << static_cast<unsigned>(monitor.state()) << "\n"; return 8;
    }
    if (monitor.pop(*block) || monitor.graph_clock() || monitor.install_worker_epoch(*epoch, {48000, 2, beatglass::ChannelLayout::stereo})) {
      std::cerr << "worker gate accepted after loss\n"; return 9;
    }
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
