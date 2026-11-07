#include "beatglass/core/file_analysis.hpp"
#include "beatglass/core/tempo_tracker.hpp"
#include <sndfile.h>
#include <algorithm>
#include <cmath>
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
  if (info.samplerate <= 0 || info.channels <= 0 || info.frames < 0) { sf_close(handle); handle = nullptr; throw std::runtime_error("Invalid audio format"); }
}
~File() { if (handle) sf_close(handle); }
File(const File&) = delete; File& operator=(const File&) = delete;
FileMetadata metadata() const { return {static_cast<std::uint32_t>(info.samplerate), static_cast<std::uint32_t>(info.channels), static_cast<std::uint64_t>(info.frames), (info.format & SF_FORMAT_TYPEMASK) == SF_FORMAT_WAV ? "WAV" : "audio"}; }
};
}
FileMetadata read_metadata(const std::filesystem::path& path) { File f(path); return f.metadata(); }
TempoResult analyze_file(const std::filesystem::path& path) {
  File file(path); constexpr std::size_t hop = 512;
  TempoTracker tracker(static_cast<std::uint32_t>(file.info.samplerate));
  std::uint64_t position = 0;
  // Select the loudest channel per block rather than averaging anti-phase stereo.
  // Channel switches can introduce discontinuities: this is a baseline, not a final downmix.
  std::vector<float> pcm(static_cast<std::size_t>(hop) * file.info.channels);
  TempoResult result{file.metadata(), std::nullopt, 0};
  std::vector<float> mono(hop);
  while (true) {
    const auto n = sf_readf_float(file.handle, pcm.data(), hop);
    if (n <= 0) { if (sf_error(file.handle) != SF_ERR_NO_ERROR) throw std::runtime_error("Audio read failed"); break; }
    int channel = 0; double best = -1;
    for (int c = 0; c < file.info.channels; ++c) { double energy = 0; for (sf_count_t i = 0; i < n; ++i) { const auto s = pcm[static_cast<std::size_t>(i)*file.info.channels+c]; energy += s*s; } if (energy > best) { best = energy; channel = c; } }
    mono.resize(static_cast<std::size_t>(n));
    for (std::size_t i = 0; i < mono.size(); ++i) mono[i] = pcm[i*file.info.channels+channel];
    tracker.process(mono, 0, position);
    position += mono.size();
  }
  const auto estimate = tracker.estimate(); result.beats = static_cast<unsigned>(estimate.beats); result.bpm = estimate.bpm;
  return result;
}
}
