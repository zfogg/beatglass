#pragma once
#include "beatglass/capture/playback_pcm_queue.hpp"
#include <cstddef>
#include <functional>
#include <span>
namespace beatglass {
/// Device-facing adapter draining copied PCM into a platform writer.
class PlaybackOutputAdapter final {
public:
  using DeviceWriter = std::function<void(const AudioBlock&)>;
  /** Construct a bounded handoff; dimensions must be nonzero. */
  PlaybackOutputAdapter(std::size_t slot_count, std::size_t samples_per_slot);
  /** Copy one worker-owned block into the bounded queue; false means rejected. */
  bool submit(const AudioBlock& block) noexcept { return queue_.push(block); }
  /// Drain one block into device-owned storage and invoke the writer.
  /// Returns false for no queued block, insufficient storage, an empty writer,
  /// or a writer exception; failed writes are not propagated.
  bool render_one(std::span<float> device_storage, const DeviceWriter& writer) noexcept;
  std::size_t queued() const noexcept { return queue_.size(); }
private: PlaybackPcmQueue queue_;
};
}
