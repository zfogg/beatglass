#pragma once
#include "beatglass/capture/playback_pcm_queue.hpp"
#include <cstddef>
#include <functional>
#include <span>
namespace beatglass {
/** Device-facing adapter draining copied PCM into a platform writer. */
class PlaybackOutputAdapter final {
public:
  using DeviceWriter = std::function<void(const AudioBlock&)>;
  PlaybackOutputAdapter(std::size_t slot_count, std::size_t samples_per_slot);
  bool submit(const AudioBlock& block) noexcept { return queue_.push(block); }
  bool render_one(std::span<float> device_storage, const DeviceWriter& writer) noexcept;
  std::size_t queued() const noexcept { return queue_.size(); }
private: PlaybackPcmQueue queue_;
};
}
