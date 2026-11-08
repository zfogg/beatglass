#pragma once
#ifdef _WIN32
#include "windows_waveout_playback.hpp"
#include "beatglass/core/playback_session.hpp"
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
namespace beatglass::windows {
/** Coordinates PlaybackSession state with the native waveOut lifecycle. */
class WindowsPlaybackSession final {
public:
  /** Open output and start a stopped session using a coordinator epoch. */
  bool start(std::uint32_t rate, std::uint16_t channels, std::size_t max_samples, std::uint64_t epoch, std::string& error);
  /** Start on an explicit waveOut device id; WAVE_MAPPER selects default. */
  bool start_device(UINT device_id, std::uint32_t rate, std::uint16_t channels, std::size_t max_samples, std::uint64_t epoch, std::string& error);
  /** Pause/stop the control session; pause leaves the native device open. */
  bool pause() noexcept;
  /** Stop and close native output. */
  void stop() noexcept;
  /** Queue decoded PCM only while the session is playing. */
  bool submit(const AudioBlock& block) noexcept;
  /** Render one queued block on the native output worker. */
  bool render(std::span<float> storage) noexcept;
  PlaybackState state() const noexcept { return session_.state(); }
private:
  PlaybackSession session_;
  WaveOutPlayback output_;
  PlaybackPcmQueue queue_{2, 4096};
};
}
#endif
