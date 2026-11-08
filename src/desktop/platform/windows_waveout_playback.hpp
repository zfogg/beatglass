#pragma once
#ifdef _WIN32
#include <windows.h>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
namespace beatglass::windows {
/** Worker-owned waveOut PCM16 writer with two preallocated completion buffers. */
class WaveOutPlayback final {
public:
  WaveOutPlayback() = default;
  ~WaveOutPlayback();
  WaveOutPlayback(const WaveOutPlayback&) = delete;
  /** Open the default output at the source format; allocates fixed buffers. */
  bool open(std::uint32_t sample_rate, std::uint16_t channels, std::size_t max_samples, std::string& error);
  /** Convert and submit one interleaved float block; worker thread only. */
  bool submit(std::span<const float> samples) noexcept;
  /** Stop, reset and release the device; safe to call repeatedly. */
  void stop() noexcept;
  bool running() const noexcept { return handle_ != nullptr; }
private:
  struct Buffer { WAVEHDR header{}; std::unique_ptr<short[]> pcm; std::atomic<bool> free{true}; };
  static void CALLBACK callback(HWAVEOUT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);
  HWAVEOUT handle_{}; std::uint16_t channels_{}; std::size_t max_samples_{}; Buffer buffers_[2];
};
}
#endif
