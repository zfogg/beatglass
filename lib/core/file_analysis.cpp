#include "beatglass/core/file_analysis.hpp"
#include <aubio/aubio.h>
#include <sndfile.h>
#include <algorithm>
#include <cmath>
#include <memory>
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
  File file(path); constexpr uint_t hop = 512, window = 1024;
  using Tempo = std::unique_ptr<aubio_tempo_t, decltype(&del_aubio_tempo)>;
  using Vec = std::unique_ptr<fvec_t, decltype(&del_fvec)>;
  Tempo tracker(new_aubio_tempo("default", window, hop, file.info.samplerate), del_aubio_tempo);
  Vec input(new_fvec(hop), del_fvec), output(new_fvec(1), del_fvec);
  if (!tracker || !input || !output) throw std::runtime_error("Could not initialize aubio");
  // Select the loudest channel per block rather than averaging anti-phase stereo.
  // Channel switches can introduce discontinuities: this is a baseline, not a final downmix.
  std::vector<float> pcm(static_cast<std::size_t>(hop) * file.info.channels);
  TempoResult result{file.metadata(), std::nullopt, 0};
  while (true) {
    const auto n = sf_readf_float(file.handle, pcm.data(), hop);
    if (n <= 0) { if (sf_error(file.handle) != SF_ERR_NO_ERROR) throw std::runtime_error("Audio read failed"); break; }
    int channel = 0; double best = -1;
    for (int c = 0; c < file.info.channels; ++c) { double energy = 0; for (sf_count_t i = 0; i < n; ++i) { const auto s = pcm[static_cast<std::size_t>(i)*file.info.channels+c]; energy += s*s; } if (energy > best) { best = energy; channel = c; } }
    for (uint_t i = 0; i < hop; ++i) input->data[i] = i < static_cast<uint_t>(n) ? pcm[static_cast<std::size_t>(i)*file.info.channels+channel] : 0;
    aubio_tempo_do(tracker.get(), input.get(), output.get());
    if (output->data[0] != 0) { ++result.beats; const auto bpm = aubio_tempo_get_bpm(tracker.get()); if (std::isfinite(bpm) && bpm > 0) result.bpm = bpm; }
  }
  return result;
}
}
