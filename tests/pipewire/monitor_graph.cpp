#include "beatglass/capture/detail/pipewire_discovery.hpp"
#include "beatglass/capture/source_lifecycle.hpp"
#include "beatglass/core/stream_continuity.hpp"
#include "beatglass/core/tracking_machine.hpp"
#include "beatglass/core/snapshot_publisher.hpp"
#include "beatglass/core/presentation_gate.hpp"
#include "beatglass/capture/detail/pipewire_monitor.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string_view>
#include <thread>
#ifdef BEATGLASS_TEST_CAPTURE_TEMPO
#include "beatglass/core/channel_reduction.hpp"
#include "beatglass/core/tempo_tracker.hpp"
#include <array>
#endif
int main(int argc, char** argv) {
  const bool missing = argc > 1 && std::string_view(argv[1]) == "missing";
  const bool tempo_test = argc > 3 && std::string_view(argv[3]) == "tempo";
#ifndef BEATGLASS_TEST_CAPTURE_TEMPO
  if (tempo_test) return 23;
#endif
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
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(missing ? 5 : tempo_test ? 25 : 12);
  auto block = std::make_unique<MonitorIngress::Block>();
  MonitorIngress::Block before_install;
  if (monitor.pop(before_install)) { std::cerr << "PCM escaped before worker installation\n"; return 11; }
  const auto state_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
  while (std::chrono::steady_clock::now() < state_deadline && monitor.state() == MonitorState::starting)
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  if (monitor.install_worker_epoch(*epoch, {48000, 2, beatglass::ChannelLayout::stereo})) {
    std::cerr << "worker installed before control consumed running event\n"; return 15;
  }
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
  beatglass::StreamContinuity continuity;
  beatglass::TrackingMachine tracking;
  beatglass::SnapshotPublisher publisher(2, 2);
  beatglass::PresentationGate presentation;
  beatglass::PublishedSnapshot retained;
  if (!tracking.start(*epoch)) return 19;
  const auto token = tracking.token();
  publisher.begin_acquisition(token);
  beatglass::Snapshot snapshot;
  bool published = false;
#ifdef BEATGLASS_TEST_CAPTURE_TEMPO
  beatglass::TempoTracker tempo(48000);
  std::array<float, 8192> mono{};
  bool tempo_acquired = false;
#endif
  bool nonzero = false;
  unsigned positive_crossings = 0;
  float previous = 0.0f;
  std::uint64_t last_end = 0;
  while (std::chrono::steady_clock::now() < deadline) {
    while (monitor.pop(*block)) {
      if (block->epoch != 1 || block->format.sample_rate != 48000 || block->format.channels != 2 ||
          block->first_frame < last_end || block->sample_count % 2) return 6;
      last_end = block->first_frame + block->sample_count / 2;
      const beatglass::AudioBlock view{block->format, block->epoch, block->first_frame, block->monotonic_ns,
                                      block->discontinuity, {block->samples.data(), block->sample_count}};
      const auto transition = continuity.observe(view);
      if (transition != beatglass::StreamTransition::acquire && transition != beatglass::StreamTransition::contiguous) return 20;
#ifdef BEATGLASS_TEST_CAPTURE_TEMPO
      if (tempo_test) {
        const auto frames = block->sample_count / 2;
        if (!beatglass::reduce_loudest_channel(view.samples, 2, {mono.data(), frames})) return 24;
        tempo.process({mono.data(), frames}, block->epoch, block->first_frame, block->discontinuity);
        const auto estimate = tempo.estimate();
        tempo_acquired = estimate.bpm && estimate.beats >= 3 && *estimate.bpm > 80 && *estimate.bpm < 160;
      }
#endif
      if (!published) {
        snapshot.observed_through = {*epoch, last_end};
        if (!tracking.accept(token) || !publisher.publish(token, snapshot) || !publisher.latest()) return 21;
        retained = publisher.latest();
        if (!presentation.apply({1, retained}) || !presentation.current()) return 27;
        published = true;
      }
      for (std::size_t i = 0; i < block->sample_count; i += 2) {
        const float sample = block->samples[i];
        if (std::abs(sample) > 0.1f) nonzero = true;
        if (previous <= 0.0f && sample > 0.0f) ++positive_crossings;
        previous = sample;
      }
    }
#ifdef BEATGLASS_TEST_CAPTURE_TEMPO
    if (tempo_test ? tempo_acquired : (nonzero && positive_crossings >= 20)) break;
#else
    if (nonzero && positive_crossings >= 20) break;
#endif
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
    while (std::chrono::steady_clock::now() < loss_deadline && monitor.state() == MonitorState::streaming) {
      if (argc > 2 && std::string_view(argv[2]) == "inject") {
        while (monitor.pop(*block)) {
          for (std::size_t i = 0; i + 1 < block->sample_count; i += 2)
            if (std::abs(block->samples[i] - block->samples[i + 1]) > 0.12f) {
              std::cerr << "unexpected-source PCM escaped before invalidation\n"; return 16;
            }
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!monitor.pop_event(event) || event.attempt.generation != attempt->generation || event.sequence != 2 || event.epoch ||
        (event.state != beatglass::SourceState::interrupted && event.state != beatglass::SourceState::error) || event.delivery_lost ||
        lifecycle.notify(event.attempt, event.sequence, event.state, event.epoch) != beatglass::SourceEventResult::accepted) {
      std::cerr << "missing ordered target-loss event state=" << static_cast<unsigned>(monitor.state()) << "\n"; return 8;
    }
#ifdef BEATGLASS_TEST_CAPTURE_TEMPO
    if (tempo_test) {
      std::array<float, 512> silence{};
      tempo.process(silence, *epoch, last_end, true);
      if (tempo.estimate().bpm || tempo.estimate().beats) { std::cerr << "tempo survived discontinuity\n"; return 25; }
    }
#endif
    continuity.stop(); tracking.stop(); publisher.clear();
    if (!presentation.apply({3, {}}) || presentation.current() || presentation.apply({2, retained}) || presentation.current()) {
      std::cerr << "stale presentation survived interruption\n"; return 28;
    }
    if (tracking.accept(token) || publisher.latest() || publisher.publish(token, snapshot)) {
      std::cerr << "stale analysis publication survived interruption\n"; return 22;
    }
    if (monitor.pop(*block) || monitor.graph_clock() || monitor.install_worker_epoch(*epoch, {48000, 2, beatglass::ChannelLayout::stereo})) {
      std::cerr << "worker gate accepted after loss\n"; return 9;
    }
    if (argc > 2 && std::string_view(argv[2]) == "remove") {
      const auto lost = std::string(argv[1]) + ".lost";
      const auto replaced = std::string(argv[1]) + ".replaced";
      { std::ofstream signal(lost); signal << "lost"; }
      const auto replacement_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
      while (std::chrono::steady_clock::now() < replacement_deadline && !std::filesystem::exists(replaced))
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      if (!std::filesystem::exists(replaced)) { std::cerr << "replacement not observed\n"; return 17; }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if (monitor.pop_event(event) || monitor.pop(*block) ||
          monitor.install_worker_epoch(*epoch, {48000, 2, beatglass::ChannelLayout::stereo})) {
        std::cerr << "replacement revived invalidated attempt\n"; return 18;
      }
    }
  }
  auto state = monitor.state();
  auto diagnostic = monitor.diagnostic_code();
  monitor.stop();
  if (missing) {
    if (nonzero || state == MonitorState::streaming) { std::cerr << "missing target fell back to another sink\n"; return 5; }
    std::cout << "missing target did not fall back\n"; return 0;
  }
#ifdef BEATGLASS_TEST_CAPTURE_TEMPO
  if (tempo_test && !tempo_acquired) { std::cerr << "aubio did not acquire captured pulse train\n"; return 26; }
#endif
  if (!nonzero || positive_crossings < 20) { std::cerr << "no output PCM, state=" << static_cast<unsigned>(state) << " diagnostic=" << diagnostic << '\n'; return 4; }
  std::cout << "observed virtual sink PCM\n";
}
