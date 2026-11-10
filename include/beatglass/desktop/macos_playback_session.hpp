#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#if defined(__APPLE__)
#include <AudioToolbox/AudioToolbox.h>
#include <beatglass/capture/playback_pcm_queue.hpp>
#include <beatglass/core/playback_session.hpp>

namespace beatglass::macos {

/** Worker-owned AudioUnit output session; the render callback only drains bounded PCM storage. */
class MacAudioUnitPlayback final {
 public:
  MacAudioUnitPlayback() = default;
  ~MacAudioUnitPlayback();
  MacAudioUnitPlayback(const MacAudioUnitPlayback&) = delete;
  MacAudioUnitPlayback& operator=(const MacAudioUnitPlayback&) = delete;

  /** Configure and start a default output unit for the requested PCM format. */
  bool start(std::uint32_t sample_rate, std::uint16_t channels,
             std::size_t max_samples, std::uint64_t epoch) noexcept;
  /** Stop and dispose the output unit; safe to call repeatedly. */
  void stop() noexcept;
  /** Queue one decoded block for the render callback. */
  bool submit(const AudioBlock& block) noexcept;
  /** Pause queue admission while retaining the current source epoch. */
  bool pause() noexcept;
  /** Resume queue admission for the current source epoch. */
  bool resume() noexcept;
  bool running() const noexcept { return unit_ != nullptr; }

 private:
  static OSStatus render(void* ref, AudioUnitRenderActionFlags*,
                         const AudioTimeStamp*, UInt32, UInt32,
                         AudioBufferList*) noexcept;
  AudioUnit unit_{nullptr};
  PlaybackPcmQueue* queue_{nullptr};
  std::unique_ptr<PlaybackPcmQueue> owned_queue_;
  std::unique_ptr<float[]> scratch_;
  std::size_t max_samples_{0};
  std::uint16_t channels_{0};
  PlaybackSession session_;
};

}  // namespace beatglass::macos
#endif
