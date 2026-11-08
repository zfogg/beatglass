#pragma once
#include "beatglass/core/audio.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
namespace beatglass {
/// Bounded SPSC copied-PCM handoff from a decode worker to a device adapter.
class PlaybackPcmQueue final {
public:
  /** Allocate slot_count slots, each able to hold samples_per_slot samples. */
  PlaybackPcmQueue(std::size_t slot_count, std::size_t samples_per_slot);
  PlaybackPcmQueue(const PlaybackPcmQueue&) = delete;
  PlaybackPcmQueue& operator=(const PlaybackPcmQueue&) = delete;
  /// Copy a borrowed block; producer-thread only, never allocates or blocks.
  /// Returns false for an empty/oversized block, zero epoch, invalid format or
  /// a frame range that would overflow. Rejected input remains caller-owned and
  /// does not alter queued blocks.
  bool push(const AudioBlock& block) noexcept;
  /// Copy one block into caller storage; consumer-thread only.
  /// A destination that is too small leaves the block queued for retry.
  bool pop(std::span<float> destination, AudioBlock& block) noexcept;
  /// Diagnostic queued-block count.
  std::size_t size() const noexcept;
private:
  struct Slot { AudioFormat format{}; std::uint64_t epoch{}, first_frame{}; std::int64_t monotonic_ns{}; bool discontinuity{}; std::unique_ptr<float[]> samples; std::size_t count{}; };
  const std::size_t capacity_, samples_per_slot_;
  std::unique_ptr<Slot[]> slots_;
  std::atomic<std::size_t> write_{0}, read_{0};
};
}
