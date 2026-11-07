#include "beatglass/core/file_analysis.hpp"
#include "beatglass/capture/file_reader.hpp"
#include "beatglass/core/tempo_tracker.hpp"
#include "beatglass/core/channel_reduction.hpp"
#include "beatglass/core/stream_continuity.hpp"
#include "beatglass/core/tracking_machine.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>
namespace beatglass {
FileMetadata read_metadata(const std::filesystem::path& path) { FileReader file(path); return file.metadata(); }
TempoResult replay_file(const std::filesystem::path& path, const ReplayObserver& observer) {
  FileReader file(path); constexpr std::size_t hop = 512;
  const auto metadata = file.metadata();
  TempoTracker tracker(metadata.sample_rate);
  std::uint64_t position = 0;
  StreamContinuity continuity;
  TrackingMachine lifecycle;
  if (!lifecycle.start(0)) throw std::runtime_error("Could not start tracking");
  std::uint64_t silent_frames = 0;
  // Select the loudest channel per block rather than averaging anti-phase stereo.
  // Channel switches can introduce discontinuities: this is a baseline, not a final downmix.
  std::vector<float> pcm(static_cast<std::size_t>(hop) * metadata.channels);
  TempoResult result{metadata, std::nullopt, 0};
  std::vector<float> mono(hop);
  while (true) {
    const auto read = file.read(pcm);
    if (read.status == FileReadStatus::end) break;
    if (read.status != FileReadStatus::data) throw std::runtime_error("Unexpected cancelled file read");
    const auto& block = read.block;
    const auto interleaved = block.samples;
    const auto n = block.samples.size() / metadata.channels;
    const auto transition = continuity.observe(block);
    if (transition == StreamTransition::invalid) throw std::runtime_error("Invalid decoded audio block");
    mono.resize(static_cast<std::size_t>(n));
    if (!reduce_loudest_channel(interleaved, metadata.channels, mono))
      throw std::runtime_error("Audio contains invalid samples");
    const bool active = std::any_of(mono.begin(), mono.end(), [](float sample) { return std::abs(sample) >= 0.0001f; });
    bool reset = transition == StreamTransition::reacquire;
    if (reset) lifecycle.discontinuity(block.epoch);
    if (active) { silent_frames = 0; lifecycle.signal(); }
    else {
      silent_frames = std::min<std::uint64_t>(metadata.sample_rate, silent_frames + mono.size());
      if (silent_frames >= static_cast<std::uint64_t>(metadata.sample_rate) && lifecycle.state() != TrackingState::silent) {
        lifecycle.silence(); reset = true;
      }
    }
    tracker.process(mono, block.epoch, position, reset);
    position += mono.size();
    const auto current = tracker.estimate();
    if (current.bpm && lifecycle.state() != TrackingState::silent) lifecycle.accept(lifecycle.token());
    result.state = lifecycle.state(); result.beats = static_cast<unsigned>(current.beats);
    result.bpm = result.state == TrackingState::tracking ? current.bpm : std::nullopt;
    if (observer) observer(position, result);
  }
  return result;
}
TempoResult analyze_file(const std::filesystem::path& path) { return replay_file(path, {}); }
}
