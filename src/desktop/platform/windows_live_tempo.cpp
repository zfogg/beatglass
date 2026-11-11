#include "windows_live_tempo.hpp"
#include "beatglass/core/channel_reduction.hpp"
#include <algorithm>
#include <cmath>
#include <span>
#include <limits>
#include <stdexcept>
#include <utility>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace beatglass::windows {
namespace {
std::optional<std::uint64_t> captureClock() {
  LARGE_INTEGER counter{}, frequency{};
  if (!QueryPerformanceCounter(&counter) || !QueryPerformanceFrequency(&frequency) || counter.QuadPart < 0 || frequency.QuadPart <= 0) return {};
  const auto value=static_cast<std::uint64_t>(counter.QuadPart), freq=static_cast<std::uint64_t>(frequency.QuadPart);
  constexpr std::uint64_t scale=10000000, max=std::numeric_limits<std::uint64_t>::max();
  const auto whole=value/freq, remainder=value%freq;
  if (whole>max/scale || remainder>max/scale) return {};
  const auto base=whole*scale, fraction=remainder*scale/freq;
  if (base>max-fraction) return {};
  return base+fraction;
}
}
LiveTempo::LiveTempo(SourceEpochAllocator& epochs) : clock_(captureClock), session_(epochs, [this] { clear(); }) {}
LiveTempo::LiveTempo(SourceEpochAllocator& epochs, std::unique_ptr<Loopback> backend, std::function<std::optional<std::uint64_t>()> clock, std::function<void()> afterProcess)
    : clock_(std::move(clock)), after_process_(std::move(afterProcess)), session_(epochs, [this] { clear(); }, std::move(backend)) {}
LiveTempo::~LiveTempo() { stop(); }
void LiveTempo::clear() noexcept {
  tracker_.reset(); bpm_.reset(); observed_.reset(); loss_stamp_.reset(); rate_ = channels_ = 0; silent_frames_ = 0;
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
    for (unsigned blocks = 0; blocks < 64 && session_.pop(packet); ++blocks) {
      if (!packet.epoch || !packet.channels || !packet.sample_count || packet.sample_count > packet.max_samples || packet.sample_count % packet.channels || !packet.sample_rate || packet.first_frame > std::numeric_limits<std::uint64_t>::max() - packet.sample_count / packet.channels) { clear(); analysis_error_ = true; session_.stop(); return false; }
      const auto fresh = [&] {
        if (!clock_) throw std::runtime_error("capture clock unavailable");
        const auto now=clock_(); if (!now) throw std::runtime_error("capture clock unavailable");
        return packet.timestamp_valid && packet.qpc_100ns<=*now && *now-packet.qpc_100ns<=10000000ULL;
      };
      if (!fresh()) { clear(); continue; }
      last_audio_ = std::chrono::steady_clock::now();
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
      const auto bpm = tracker_->estimate().bpm;
      if (after_process_) after_process_();
      const auto before_poll=invalidation_generation_;
      session_.poll();
      if (before_poll != invalidation_generation_ || session_.state() != SourceState::running) break;
      const auto token=session_.lossToken();
      if (!fresh() || !token || !token->matches(packet.loss_stamp)) { clear(); break; }
      loss_stamp_ = packet.loss_stamp;
      observed_ = EventTime{packet.epoch, packet.first_frame + frames};
      bpm_ = bpm;
    }
    if (std::chrono::steady_clock::now() - last_audio_ > std::chrono::seconds(1)) clear();
    return !generation_exhausted_ && session_.state() != SourceState::error && session_.state() != SourceState::interrupted;
  } catch (...) {
    clear(); analysis_error_ = true; session_.stop(); return false;
  }
}
std::optional<float> LiveTempo::bpm() const { return bpm_; }
std::optional<EventTime> LiveTempo::observedThrough() const { return observed_; }
StampedLiveTempo LiveTempo::stampedTempo() const noexcept {
  if (!bpm_ || !observed_ || !loss_stamp_) return {{}, {}, session_.lossToken(), {}};
  return {bpm_, observed_, session_.lossToken(), loss_stamp_};
}
std::shared_ptr<const LoopbackLossToken> LiveTempo::lossToken() const noexcept { return session_.lossToken(); }
std::optional<std::uint64_t> LiveTempo::lossStamp() const noexcept { return loss_stamp_; }
std::uint64_t LiveTempo::invalidationGeneration() const { return invalidation_generation_; }
SourceState LiveTempo::state() const { return session_.state(); }
std::int32_t LiveTempo::error_code() const { return session_.error_code(); }
void LiveTempo::stop() { session_.stop(); clear(); }
}
