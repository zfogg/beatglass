#include "beatglass/core/music_collector.hpp"
#include <stdexcept>
#include <utility>
namespace beatglass {
struct MusicCollector::Impl {
  explicit Impl(std::uint64_t e, MusicCollectorLimits l) : epoch(e), limits(l) { value.epoch=e; }
  std::uint64_t epoch;
  MusicCollectorLimits limits;
  CausalMusicReplay replay;
  OfflineAnalysis value;
  bool failed{}, finished{}, seen{};
};
MusicCollector::MusicCollector(std::uint64_t epoch, MusicCollectorLimits limits) {
  if (!epoch) throw std::invalid_argument("collector epoch must be nonzero");
  impl_=std::make_unique<Impl>(epoch, limits);
}
MusicCollector::~MusicCollector() = default;
void MusicCollector::process(const AudioBlock& block) {
  auto& s=*impl_;
  if (s.failed || s.finished) throw std::logic_error("collector unavailable");
  try {
    if (block.epoch != s.epoch || block.first_frame != s.value.observed_through || block.discontinuity ||
        (s.seen && block.format.sample_rate != s.value.sample_rate))
      throw std::invalid_argument("collector requires one contiguous epoch");
    auto state=s.replay.process(block, [&](const CausalMusicBeat& b) {
      if (s.value.beats.size() >= s.limits.max_beats) throw std::length_error("beat limit exceeded");
      s.value.beats.push_back({b.frame,b.observed_through,b.strength});
    }, [&](const CausalTonalFrame& t) {
      if (s.value.tonal_windows.size() >= s.limits.max_tonal_windows ||
          (t.rolling_key && t.rolling_key->size()>s.limits.max_label_bytes) ||
          (t.local_chord && t.local_chord->size()>s.limits.max_label_bytes))
        throw std::length_error("tonal limit exceeded");
      OfflineTonalWindow w;
      w.start_frame=t.first_frame; w.end_frame=t.observed_through; w.rolling_key_from=t.rolling_key_from;
      if (t.rolling_key) w.rolling_key=OfflineEstimate{*t.rolling_key,t.rolling_key_strength};
      if (t.local_chord) w.local_chord=OfflineEstimate{*t.local_chord,t.local_chord_strength};
      s.value.tonal_windows.push_back(std::move(w));
    });
    s.value.sample_rate=state.sample_rate; s.value.observed_through=state.tempo.observed_through;
    s.value.analyzed_through=state.tempo.analyzed_through; s.value.bpm=state.tempo.bpm; s.seen=true;
  } catch (...) { s.failed=true; s.value={}; throw; }
}
UnverifiedMusicAnalysis MusicCollector::finish() {
  auto& s=*impl_;
  if (s.failed || s.finished || !s.seen) throw std::logic_error("collector has no complete result");
  s.finished=true; return {std::move(s.value)};
}
}
