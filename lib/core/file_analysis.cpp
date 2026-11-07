#include "beatglass/core/file_analysis.hpp"
#include "beatglass/core/tempo_tracker.hpp"
#include "beatglass/core/channel_reduction.hpp"
#include "beatglass/core/stream_continuity.hpp"
#include <sndfile.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>
namespace beatglass {
namespace {
struct File { SNDFILE* handle; SF_INFO info{}; explicit File(const std::filesystem::path& path) : handle(nullptr) {
#ifdef _WIN32
  handle = sf_wchar_open(path.c_str(), SFM_READ, &info);
#else
  handle = sf_open(path.c_str(), SFM_READ, &info);
#endif
  if (!handle) throw std::runtime_error("Could not open audio file");
  if (info.samplerate <= 0 || info.channels <= 0 || info.channels > std::numeric_limits<std::uint16_t>::max() || info.frames < 0) { sf_close(handle); handle = nullptr; throw std::runtime_error("Invalid audio format"); }
}
~File() { if (handle) sf_close(handle); }
File(const File&) = delete; File& operator=(const File&) = delete;
FileMetadata metadata() const { return {static_cast<std::uint32_t>(info.samplerate), static_cast<std::uint32_t>(info.channels), static_cast<std::uint64_t>(info.frames), (info.format & SF_FORMAT_TYPEMASK) == SF_FORMAT_WAV ? "WAV" : "audio"}; }
};
}
FileMetadata read_metadata(const std::filesystem::path& path) { File f(path); return f.metadata(); }
TempoResult replay_file(const std::filesystem::path& path, const ReplayObserver& observer) {
  File file(path); constexpr std::size_t hop = 512;
  TempoTracker tracker(static_cast<std::uint32_t>(file.info.samplerate));
  std::uint64_t position = 0;
  StreamContinuity continuity;
  // Select the loudest channel per block rather than averaging anti-phase stereo.
  // Channel switches can introduce discontinuities: this is a baseline, not a final downmix.
  std::vector<float> pcm(static_cast<std::size_t>(hop) * file.info.channels);
  TempoResult result{file.metadata(), std::nullopt, 0};
  std::vector<float> mono(hop);
  while (true) {
    const auto n = sf_readf_float(file.handle, pcm.data(), hop);
    if (n <= 0) { if (sf_error(file.handle) != SF_ERR_NO_ERROR) throw std::runtime_error("Audio read failed"); break; }
    const auto interleaved = std::span(pcm.data(), static_cast<std::size_t>(n) * file.info.channels);
    AudioBlock block{{static_cast<std::uint32_t>(file.info.samplerate), static_cast<std::uint16_t>(file.info.channels)}, 0, position, 0, false, interleaved};
    const auto transition = continuity.observe(block);
    if (transition == StreamTransition::invalid) throw std::runtime_error("Invalid decoded audio block");
    mono.resize(static_cast<std::size_t>(n));
    if (!reduce_loudest_channel(interleaved, file.info.channels, mono))
      throw std::runtime_error("Audio contains invalid samples");
    tracker.process(mono, 0, position, transition == StreamTransition::reacquire);
    position += mono.size();
    const auto current = tracker.estimate(); result.beats = static_cast<unsigned>(current.beats); result.bpm = current.bpm;
    if (observer) observer(position, result);
  }
  const auto estimate = tracker.estimate(); result.beats = static_cast<unsigned>(estimate.beats); result.bpm = estimate.bpm;
  return result;
}
TempoResult analyze_file(const std::filesystem::path& path) { return replay_file(path, {}); }
}
