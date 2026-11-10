#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include "beatglass/core/audio.hpp"

namespace beatglass {
// One producer and one consumer. Slots are allocated with the queue. A failed
// push drops the entire block; the next successful push reports discontinuity.
// The producer must not be called concurrently, including during shutdown.
/// Bounded preallocated single-producer, single-consumer queue. One slot is
/// reserved; the producer must not run concurrently, including shutdown. A
/// rejected block is dropped and the next successful push marks a
/// discontinuity. Atomics must be lock-free.
template<std::size_t Slots, std::size_t MaxSamples> class AudioQueue {
  static_assert(Slots >= 2 && MaxSamples > 0);
  static_assert(std::atomic<std::size_t>::is_always_lock_free &&
                std::atomic<std::uint64_t>::is_always_lock_free,
                "Audio callback requires lock-free atomics");
public:
/// Consumer-owned copy of one block, including sample count and timing.
  struct Block {
    AudioFormat format{};
    std::uint64_t epoch{}, first_frame{};
    std::int64_t monotonic_ns{};
    bool discontinuity{};
    std::size_t sample_count{};
    std::array<float, MaxSamples> samples{};
  };
/// Copy a block on the producer thread. Reject invalid, oversized, full-queue,
/// zero-epoch, or overflowing-frame-range input; count complete frames from a
/// rejected block as dropped (with saturating accounting) and mark the next
/// accepted block discontinuous. The callback-safe path performs no allocation,
/// blocking, logging, or callbacks.
  bool push(const AudioBlock& input) noexcept {
    const auto frames = input.format.channels ? input.samples.size() / input.format.channels : 0;
    const auto write = write_.load(std::memory_order_relaxed);
    const auto next = (write + 1) % Slots;
    if (input.samples.empty() || !input.epoch || !input.format.channels || !input.format.sample_rate || input.samples.size() % input.format.channels ||
        input.samples.size() > MaxSamples || frames > std::numeric_limits<std::uint64_t>::max() - input.first_frame ||
        next == read_.load(std::memory_order_acquire)) {
      dropped_.fetch_add(frames, std::memory_order_relaxed);
      discontinuity_ = true;
      return false;
    }
    auto& out = slots_[write];
    out.format = input.format; out.epoch = input.epoch; out.first_frame = input.first_frame;
    out.monotonic_ns = input.monotonic_ns; out.discontinuity = input.discontinuity || discontinuity_;
    out.sample_count = input.samples.size();
    for (std::size_t i = 0; i < input.samples.size(); ++i) out.samples[i] = input.samples[i];
    discontinuity_ = false;
    write_.store(next, std::memory_order_release);
    return true;
  }
  // Copies into consumer-owned storage; no slot pointer escapes.
/// Copy the next block into consumer-owned storage; return false if empty.
/// The destination is fully overwritten only on success; an empty queue leaves
/// the caller's output object unchanged.
  bool pop(Block& out) noexcept {
    const auto read = read_.load(std::memory_order_relaxed);
    if (read == write_.load(std::memory_order_acquire)) return false;
    out = slots_[read];
    read_.store((read + 1) % Slots, std::memory_order_release);
    return true;
  }
/// Return the current number of queued blocks for consumer-side telemetry.
/// The value is a snapshot and may change immediately after return; it never
/// participates in producer admission decisions.
  std::size_t size() const noexcept {
    const auto read = read_.load(std::memory_order_acquire);
    const auto write = write_.load(std::memory_order_acquire);
    return (write + Slots - read) % Slots;
  }
/// Return the cumulative number of complete frames counted in rejected blocks.
  std::uint64_t dropped_frames() const noexcept { return dropped_.load(std::memory_order_relaxed); }
  /// Return the number of queued blocks currently available to the consumer.
  /// The result is a snapshot and may change immediately after this call.
  std::size_t size() const noexcept {
    const auto read = read_.load(std::memory_order_acquire);
    const auto write = write_.load(std::memory_order_acquire);
    return write >= read ? write - read : Slots - read + write;
  }
private:
  std::array<Block, Slots> slots_{};
  std::atomic<std::size_t> read_{0}, write_{0};
  std::atomic<std::uint64_t> dropped_{0};
  bool discontinuity_{false}; // producer-owned
};
}
