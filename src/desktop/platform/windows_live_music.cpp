#include "windows_live_music.hpp"
#include <algorithm>
#include <limits>
#include <span>
#include <utility>
namespace beatglass::windows {
LiveMusic::LiveMusic(SourceEpochAllocator& epochs) : session_(epochs,[this]{ clear(); }) {}
LiveMusic::LiveMusic(SourceEpochAllocator& epochs,std::unique_ptr<Loopback> backend)
    : session_(epochs,[this]{ clear(); },std::move(backend)) {}
LiveMusic::~LiveMusic() { stop(); }
void LiveMusic::invalidate_publication() noexcept {
  latest_.reset();
  if (generation_==std::numeric_limits<std::uint64_t>::max() ||
      revision_==std::numeric_limits<std::uint64_t>::max()) exhausted_=true;
  else { ++generation_; ++revision_; }
}
void LiveMusic::clear() noexcept { replay_.reset(); invalidate_publication(); }
bool LiveMusic::start(const std::string& endpoint) { return start(endpoint, {}); }
bool LiveMusic::start(const std::string& endpoint, std::shared_ptr<LoopbackStartAuthorization> authorization) {
  if (exhausted_ || session_.state()!=SourceState::stopped) return false;
  clear(); failed_=false; last_audio_=std::chrono::steady_clock::now();
  if (exhausted_) return false;
  return session_.start(endpoint, std::move(authorization));
}
bool LiveMusic::pump() {
  if (failed_ || exhausted_) return false;
  try {
    session_.poll();
    LoopbackPacket packet;
    for (unsigned n=0; n<16 && session_.pop(packet); ++n) {
      if (exhausted_) break;
      if (!packet.epoch || !packet.sample_rate || !packet.channels ||
          packet.channels>std::numeric_limits<std::uint16_t>::max() ||
          !packet.sample_count || packet.sample_count>packet.max_samples ||
          packet.sample_count%packet.channels) throw std::invalid_argument("invalid capture packet");
      last_audio_=std::chrono::steady_clock::now();
      if (!replay_) replay_=std::make_unique<CausalMusicReplay>();
      const auto prior=replay_->state().tempo;
      if (prior.epoch && (packet.epoch!=prior.epoch || packet.first_frame!=prior.observed_through))
        invalidate_publication();
      const auto frames=packet.sample_count/packet.channels;
      // Each sub-block completes at most one internal hop, exposing every
      // silence and resume transition even when both occur in one packet.
      for (std::uint32_t offset=0; offset<frames;) {
        const auto count=(std::min)(std::uint32_t{512},frames-offset);
        const auto before=replay_->state().tempo.state;
        AudioBlock block{{packet.sample_rate,static_cast<std::uint16_t>(packet.channels)},
          packet.epoch,packet.first_frame+offset,0,
          offset==0 && (packet.discontinuity || !packet.timestamp_valid),
          std::span<const float>(packet.samples.data()+offset*packet.channels,count*packet.channels)};
        auto state=replay_->process(block);
        const auto after=state.tempo.state;
        if ((before==TrackingState::silent)!=(after==TrackingState::silent)) invalidate_publication();
        if (exhausted_) throw std::overflow_error("publication generation exhausted");
        if ((state.rolling_key && state.rolling_key->size()>64) ||
            (state.local_chord && state.local_chord->size()>64)) throw std::length_error("label too long");
        if (revision_==std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("publication revision exhausted");
        ++revision_; latest_=std::move(state); offset+=count;
      }
    }
    if (latest_ && std::chrono::steady_clock::now()-last_audio_>std::chrono::seconds(1)) clear();
    if (exhausted_ || session_.state()==SourceState::error || session_.state()==SourceState::interrupted) {
      clear(); failed_=true; session_.stop(); return false;
    }
    return true;
  } catch (...) { clear(); failed_=true; session_.stop(); return false; }
}
void LiveMusic::stop() { session_.stop(); clear(); }
}
