#include "beatglass/core/music_replay.hpp"
#include "beat_tracker.hpp"
#include "tonal_tracker.hpp"
#include <stdexcept>
namespace beatglass {
struct CausalMusicReplay::Impl {
  std::unique_ptr<CausalTempoReplay> tempo;
  std::unique_ptr<detail::BeatTracker> beat;
  std::unique_ptr<detail::TonalTracker> tonal;
  std::uint32_t rate{};
  bool failed{}, has_source{}, require_new_epoch{};
  std::uint64_t interrupted_epoch{};
  CausalMusicState result;
  void clear_tonal() { result.rolling_key.reset(); result.local_chord.reset(); result.rolling_key_from.reset(); result.tonal_from.reset(); result.tonal_through.reset(); result.rolling_key_strength.reset(); result.local_chord_strength.reset(); }
};
CausalMusicReplay::CausalMusicReplay() : impl_(std::make_unique<Impl>()) {}
CausalMusicReplay::~CausalMusicReplay() = default;
CausalMusicState CausalMusicReplay::process(const AudioBlock& block, const BeatObserver& observer, const TonalObserver& tonal_observer) {
  auto& s = *impl_;
  if (s.failed) throw std::runtime_error("Music replay failed");
  try {
    if (s.require_new_epoch && block.epoch <= s.interrupted_epoch) throw std::invalid_argument("A new epoch is required after interruption");
    if (!s.tempo) s.tempo = std::make_unique<CausalTempoReplay>();
    if (s.has_source && (block.epoch != s.result.tempo.epoch || block.first_frame != s.result.tempo.observed_through || block.discontinuity)) s.clear_tonal();
    auto tempo = s.tempo->process(block, [&](const CausalMonoHop& hop, TrackingState state) {
      if (state == TrackingState::silent) { s.clear_tonal(); return; }
      if (!s.beat || !s.tonal || s.rate != hop.sample_rate) {
        s.beat = std::make_unique<detail::BeatTracker>(hop.sample_rate);
        s.tonal = std::make_unique<detail::TonalTracker>(hop.sample_rate);
        s.rate = hop.sample_rate; s.clear_tonal();
      } else if (hop.reset) s.clear_tonal();
      s.beat->process(hop.samples, hop.epoch, hop.first_frame, [&](const detail::BeatEvent& event) {
        if (observer) observer({event.epoch, event.frame, event.observed_through, hop.sample_rate, std::nullopt});
      }, hop.reset);
      s.tonal->process(hop.samples, hop.epoch, hop.first_frame, hop.reset);
      const auto tonal = s.tonal->estimate();
      const bool fresh = tonal.observed_through && (!s.result.tonal_through || tonal.observed_through > *s.result.tonal_through);
      s.result.tonal_from = tonal.observed_through ? std::optional<std::uint64_t>(tonal.observed_from) : std::nullopt;
      if (fresh && tonal_observer) tonal_observer({hop.epoch, tonal.observed_from, tonal.observed_through, tonal.key ? std::optional<std::uint64_t>(tonal.key_from) : std::nullopt, tonal.key, tonal.chord, tonal.key ? std::optional<float>(tonal.key_strength) : std::nullopt, tonal.chord ? std::optional<float>(tonal.chord_strength) : std::nullopt, hop.sample_rate});
      s.result.rolling_key_from = tonal.key ? std::optional<std::uint64_t>(tonal.key_from) : std::nullopt;
      s.result.rolling_key = tonal.key; s.result.local_chord = tonal.chord; s.result.tonal_through = tonal.observed_through ? std::optional<std::uint64_t>(tonal.observed_through) : std::nullopt;
      s.result.rolling_key_strength = tonal.key ? std::optional<float>(tonal.key_strength) : std::nullopt; s.result.local_chord_strength = tonal.chord ? std::optional<float>(tonal.chord_strength) : std::nullopt;
    });
    s.result.tempo = tempo; s.result.sample_rate = block.format.sample_rate; s.has_source = true; s.require_new_epoch = false; return s.result;
  } catch (...) { s.failed = true; s.result = {}; s.result.tempo.state = TrackingState::error; throw; }
}
void CausalMusicReplay::interrupt() noexcept {
  auto& s = *impl_; if (s.failed) return;
  if (s.has_source) { s.interrupted_epoch = s.result.tempo.epoch; s.require_new_epoch = true; }
  s.tempo.reset(); s.beat.reset(); s.tonal.reset(); s.rate = 0; s.has_source = false; s.result = {};
}
CausalMusicState CausalMusicReplay::state() const { return impl_->result; }
}
