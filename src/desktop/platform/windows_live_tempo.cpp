#include "windows_live_tempo.hpp"
#include "beatglass/core/channel_reduction.hpp"
#include <algorithm>
#include <cmath>
#include <span>
#include <limits>

namespace beatglass::windows {
LiveTempo::LiveTempo(SourceEpochAllocator& epochs) : session_(epochs, [this] { clear(); }) {}
LiveTempo::~LiveTempo() { stop(); }
void LiveTempo::clear() noexcept {
  tracker_.reset(); bpm_.reset(); observed_.reset(); rate_ = channels_ = 0; silent_frames_ = 0;
  if (invalidation_generation_ == std::numeric_limits<std::uint64_t>::max()) generation_exhausted_ = true;
  else ++invalidation_generation_;
}
bool LiveTempo::start(const std::string& endpoint) {
  if (generation_exhausted_ || session_.state() != SourceState::stopped) return false;
  clear(); analysis_error_ = false; last_audio_ = std::chrono::steady_clock::now();
  return session_.start(endpoint);
}
bool LiveTempo::pump() {
  if (analysis_error_ || generation_exhausted_) return false;
  try {
    session_.poll();
    LoopbackPacket packet;
    for (unsigned blocks = 0; blocks < 16 && session_.pop(packet); ++blocks) {
      last_audio_ = std::chrono::steady_clock::now();
      if (!packet.channels || packet.sample_count % packet.channels || !packet.sample_rate) { clear(); analysis_error_ = true; session_.stop(); return false; }
      if (rate_ != packet.sample_rate || channels_ != packet.channels) {
        clear(); rate_ = packet.sample_rate; channels_ = packet.channels;
      }
      const auto frames = packet.sample_count / packet.channels;
      mono_.resize(frames);
      if (!reduce_loudest_channel(std::span(packet.samples.data(), packet.sample_count), packet.channels, mono_)) {
        clear(); analysis_error_ = true; session_.stop(); return false;
      }
      bool signal = false;
      for (const auto sample : mono_) if (std::abs(sample) > 0.0001f) { signal = true; break; }
      if (signal) silent_frames_ = 0;
      else silent_frames_ = (std::min)(std::uint64_t(rate_), silent_frames_ + frames);
      if (silent_frames_ >= rate_) { clear(); continue; }
      if (!tracker_) tracker_ = std::make_unique<TempoTracker>(rate_);
      tracker_->process(mono_, packet.epoch, packet.first_frame, packet.discontinuity);
      observed_ = EventTime{packet.epoch, packet.first_frame + frames};
      bpm_ = tracker_->estimate().bpm;
    }
    if (std::chrono::steady_clock::now() - last_audio_ > std::chrono::seconds(1)) clear();
    return !generation_exhausted_ && session_.state() != SourceState::error && session_.state() != SourceState::interrupted;
  } catch (...) {
    clear(); analysis_error_ = true; session_.stop(); return false;
  }
}
std::optional<float> LiveTempo::bpm() const { return bpm_; }
std::optional<EventTime> LiveTempo::observedThrough() const { return observed_; }
std::uint64_t LiveTempo::invalidationGeneration() const { return invalidation_generation_; }
SourceState LiveTempo::state() const { return session_.state(); }
void LiveTempo::stop() { session_.stop(); clear(); }
}
