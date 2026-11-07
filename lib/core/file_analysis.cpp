#include "beatglass/core/file_analysis.hpp"
#include "beatglass/capture/file_reader.hpp"
#include "beatglass/core/causal_replay.hpp"
#include <limits>
#include <stdexcept>
#include <vector>
namespace beatglass {
FileMetadata read_metadata(const std::filesystem::path& path) { FileReader file(path); return file.metadata(); }
TempoResult replay_file(const std::filesystem::path& path, const ReplayObserver& observer) {
  FileReader file(path); constexpr std::size_t hop = 512;
  const auto metadata = file.metadata();
  CausalTempoReplay replay;
  std::vector<float> pcm(hop * metadata.channels);
  TempoResult result{metadata, std::nullopt, 0};
  while (true) {
    const auto read = file.read(pcm);
    if (read.status == FileReadStatus::end) break;
    if (read.status != FileReadStatus::data) throw std::runtime_error("Unexpected cancelled file read");
    const auto state = replay.process(read.block);
    if (state.beats > std::numeric_limits<unsigned>::max()) throw std::overflow_error("Beat count overflow");
    result.state = state.state; result.beats = static_cast<unsigned>(state.beats); result.bpm = state.bpm;
    if (observer) observer(state.observed_through, result);
  }
  return result;
}
TempoResult analyze_file(const std::filesystem::path& path) { return replay_file(path, {}); }
}
