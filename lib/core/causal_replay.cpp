#include "beatglass/core/causal_replay.hpp"
#include "beatglass/core/channel_reduction.hpp"
#include "beatglass/core/stream_continuity.hpp"
#include "beatglass/core/tempo_tracker.hpp"
#include "beatglass/core/tracking_machine.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>
namespace beatglass {
struct CausalTempoReplay::Impl {
  static constexpr std::size_t hop = 512;
  StreamContinuity continuity;
  TrackingMachine lifecycle;
  std::unique_ptr<TempoTracker> tracker;
  std::vector<float> interleaved, mono;
  std::size_t used{};
  std::uint64_t hop_first{}, silent_frames{};
  std::uint32_t rate{}; std::uint16_t channels{};
  CausalTempoState result;
  bool failed{}, reset_next{}, processing{};
  void begin(const AudioBlock& block, StreamTransition transition) {
    if (transition == StreamTransition::acquire) {
      if (!lifecycle.start(block.epoch)) throw std::runtime_error("Could not start tracking");
    } else if (!lifecycle.discontinuity(block.epoch)) throw std::runtime_error("Could not reacquire tracking");
    rate = block.format.sample_rate; channels = block.format.channels;
    tracker = std::make_unique<TempoTracker>(rate);
    interleaved.assign(hop * channels, 0); mono.resize(hop);
    used = 0; hop_first = block.first_frame; silent_frames = 0; reset_next = true;
    result = {}; result.epoch = block.epoch; result.observed_through = result.analyzed_through = block.first_frame;
    result.state = lifecycle.state();
  }
  void analyze_hop(const CausalTempoReplay::HopObserver& observer) {
    if (!reduce_loudest_channel(interleaved, channels, mono)) throw std::runtime_error("Audio contains invalid samples");
    const bool active = std::any_of(mono.begin(), mono.end(), [](float sample) { return std::abs(sample) >= 0.0001f; });
    bool reset = false;
    const bool resumed = active && lifecycle.state() == TrackingState::silent;
    if (active) { silent_frames = 0; if (!lifecycle.signal()) throw std::runtime_error("Could not resume tracking"); }
    else {
      silent_frames = std::min<std::uint64_t>(rate, silent_frames + hop);
      if (silent_frames >= rate && lifecycle.state() != TrackingState::silent) {
        if (!lifecycle.silence()) throw std::runtime_error("Could not invalidate tracking");
        reset = true;
      }
    }
    tracker->process(mono, result.epoch, hop_first, reset);
    const auto first = hop_first;
    hop_first += hop; result.analyzed_through = hop_first;
    const auto current = tracker->estimate();
    if (current.bpm && lifecycle.state() != TrackingState::silent && !lifecycle.accept(lifecycle.token())) throw std::runtime_error("Could not accept tracking evidence");
    result.state = lifecycle.state(); result.beats = current.beats; result.beat_frames = current.beat_frames;
    result.bpm = result.state == TrackingState::tracking ? current.bpm : std::nullopt;
    if (observer) observer({result.epoch, first, rate, reset_next || reset || resumed, mono}, result.state);
    if (failed) throw std::logic_error("Replay observer reentered process");
    reset_next = false;
  }
};
CausalTempoReplay::CausalTempoReplay() : impl_(std::make_unique<Impl>()) {}
CausalTempoReplay::~CausalTempoReplay() = default;
CausalTempoState CausalTempoReplay::process(const AudioBlock& block, const HopObserver& observer) {
  auto& s = *impl_;
  if (s.failed) throw std::runtime_error("Replay engine failed");
  if (s.processing) { s.failed = true; throw std::logic_error("Replay observer reentered process"); }
  struct ProcessingGuard { bool& flag; ~ProcessingGuard() { flag = false; } } guard{s.processing};
  s.processing = true;
  try {
    if (block.epoch == 0) throw std::invalid_argument("Audio block requires a nonzero epoch");
    if (!std::all_of(block.samples.begin(), block.samples.end(), [](float x) { return std::isfinite(x); })) throw std::invalid_argument("Audio contains invalid samples");
    if (s.tracker && (block.epoch < s.result.epoch ||
        (block.epoch == s.result.epoch && block.first_frame < s.result.observed_through)))
      throw std::invalid_argument("Stale audio block");
    const auto transition = s.continuity.observe(block);
    if (transition == StreamTransition::invalid) throw std::invalid_argument("Invalid audio block");
    if (transition != StreamTransition::contiguous) s.begin(block, transition);
    const auto frames = block.samples.size() / s.channels;
    std::size_t consumed = 0;
    while (consumed < frames) {
      const auto count = std::min(Impl::hop - s.used, frames - consumed);
      std::copy_n(block.samples.begin() + consumed * s.channels, count * s.channels, s.interleaved.begin() + s.used * s.channels);
      consumed += count; s.used += count;
      if (s.used == Impl::hop) { s.analyze_hop(observer); s.used = 0; }
    }
    s.result.observed_through = block.first_frame + frames;
    return s.result;
  } catch (...) { s.failed = true; s.lifecycle.fail(); s.result = {}; s.result.state = TrackingState::error; throw; }
}
CausalTempoState CausalTempoReplay::state() const noexcept { return impl_->result; }
}
