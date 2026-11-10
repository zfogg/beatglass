#include "beatglass/core/file_analysis.hpp"
#include "beatglass/capture/file_reader.hpp"
#include "beatglass/core/causal_replay.hpp"
#include "beat_tracker.hpp"
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>
namespace beatglass {
FileMetadata read_metadata(const std::filesystem::path& path) {
  FileReader file(path, 1);
  return file.metadata();
}
TempoResult replay_file(const std::filesystem::path& path, const ReplayObserver& observer,
                        const ReplayBeatObserver& beat_observer, std::stop_token cancel) {
  // A file replay is a source in its own right and must have a nonzero epoch.
  FileReader file(path, 1); constexpr std::size_t hop = 512;
  const auto metadata = file.metadata();
  CausalTempoReplay replay;
  std::unique_ptr<detail::BeatTracker> beats;
  std::vector<float> pcm(hop * metadata.channels);
  TempoResult result{metadata, std::nullopt, 0};
  while (true) {
    const auto read = file.read(pcm, cancel);
    if (read.status == FileReadStatus::end) break;
    if (read.status == FileReadStatus::cancelled) return result;
    if (read.status != FileReadStatus::data) throw std::runtime_error("Unexpected cancelled file read");
    if (beat_observer && !beats) beats = std::make_unique<detail::BeatTracker>(read.block.format.sample_rate);
    const auto state = replay.process(read.block, [&](const CausalMonoHop& hop, TrackingState) {
      if (beats) beats->process(hop.samples, hop.epoch, hop.first_frame,
        [&](const detail::BeatEvent& event) { beat_observer({event.epoch, event.frame, event.observed_through}); }, hop.reset);
    });
    if (state.beats > std::numeric_limits<unsigned>::max()) throw std::overflow_error("Beat count overflow");
    result.state = state.state; result.beats = static_cast<unsigned>(state.beats); result.bpm = state.bpm;
    if (observer) observer(state.observed_through, result);
  }
  return result;
}
TempoResult analyze_file(const std::filesystem::path& path, std::stop_token cancel) {
  return replay_file(path, {}, {}, cancel);
}
}
