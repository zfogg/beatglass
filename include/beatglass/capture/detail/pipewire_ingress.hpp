#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include "beatglass/core/audio_queue.hpp"
namespace beatglass::pipewire {
/** Result of validating and copying one interleaved stereo float buffer. */
enum class IngressResult { copied, dropped, empty, invalid };
/**
 * Provisional single-producer/single-consumer capture ingress for fixed 48 kHz
 * interleaved stereo float PCM. Construct with a process-unique nonzero epoch.
 * One producer calls ingest; one worker calls pop. Destruction requires both
 * threads quiesced. Copying uses preallocated bounded storage and no locks.
 */
class MonitorIngress {
public:
  /** Consumer-owned audio copy. Its monotonic_ns is unavailable, not an anchor. */
  using Block = AudioQueue<32, 16384>::Block;
  /** Construct a fresh ingress for one externally allocated epoch. */
  explicit MonitorIngress(std::uint64_t epoch) noexcept : epoch_(epoch) {}
  /**
   * Validate byte bounds, float alignment, stride and complete frames before
   * copying. Invalid or corrupted input is fatal to this epoch; caller must
   * stop and reacquire. Full or oversized queue blocks are dropped as a whole;
   * their frame positions advance and the next accepted block marks a gap.
   * No allocation, locking, logging or callback into user code occurs here.
   */
  IngressResult ingest(std::span<const std::byte> data, std::size_t offset, std::size_t size,
                       std::int32_t stride, bool corrupted, bool discontinuity) noexcept;
  /** Copy the next block into worker-owned storage; never retain a slot pointer. */
  bool pop(Block& block) noexcept { return queue_.pop(block); }
  /** Cumulative complete frames dropped on queue overflow or oversized blocks. */
  std::uint64_t dropped_frames() const noexcept { return queue_.dropped_frames(); }
private:
  AudioQueue<32, 16384> queue_{};
  std::uint64_t epoch_{}, next_frame_{};
  bool failed_{};
};
}
