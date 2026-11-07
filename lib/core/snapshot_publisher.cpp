#include "beatglass/core/snapshot_publisher.hpp"
#include <algorithm>
#include <utility>
namespace beatglass {
namespace {
template<class T> void keep_last(std::vector<T>& items, std::size_t limit) {
  const auto n = std::min(items.size(), limit);
  std::vector<T> bounded(items.end() - static_cast<std::ptrdiff_t>(n), items.end());
  items = std::move(bounded);
}
bool valid_candidates(const std::vector<Candidate>& values) {
  return std::all_of(values.begin(), values.end(), [](const auto& v) { return v.label.size() <= 128; });
}
}
bool SnapshotPublisher::publish(AcquisitionToken token, Snapshot snapshot) {
  const auto through = snapshot.observed_through;
  if (!active_ || token.epoch != epoch_ || token.generation != generation_ || through.epoch != epoch_ || (have_frame_ && through.frame < last_frame_) ||
      snapshot.estimator_version.size() > 128 || snapshot.configuration_id.size() > 128 ||
      !valid_candidates(snapshot.tempo_candidates) || !valid_candidates(snapshot.key_candidates)) return false;
  std::uint64_t previous = 0; bool first = true;
  for (const auto& beat : snapshot.beats) {
    if (beat.time.epoch != epoch_ || beat.time.frame > through.frame || (!first && beat.time.frame < previous)) return false;
    first = false; previous = beat.time.frame;
  }
  first = true; previous = 0;
  for (const auto& chord : snapshot.chords) {
    if (chord.start.epoch != epoch_ || chord.end.epoch != epoch_ || chord.start.frame > chord.end.frame ||
        chord.end.frame > through.frame || (!first && chord.start.frame < previous) || chord.chord.label.size() > 128) return false;
    first = false; previous = chord.end.frame;
  }
  keep_last(snapshot.beats, history_limit_); keep_last(snapshot.chords, history_limit_);
  keep_last(snapshot.tempo_candidates, candidate_limit_); keep_last(snapshot.key_candidates, candidate_limit_);
  auto published = std::make_shared<const Snapshot>(std::move(snapshot));
  last_frame_ = through.frame; have_frame_ = true;
  std::atomic_store_explicit(&latest_, std::move(published), std::memory_order_release);
  return true;
}
}
