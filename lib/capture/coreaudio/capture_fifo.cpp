#include "beatglass/capture/coreaudio_fifo_internal.hpp"
#include <algorithm>
#include <limits>

namespace beatglass::coreaudio {
static_assert(std::atomic<std::size_t>::is_always_lock_free && std::atomic<std::uint64_t>::is_always_lock_free && std::atomic<bool>::is_always_lock_free);
CaptureFifo::CaptureFifo(std::uint64_t epoch, std::uint32_t sample_rate, std::uint32_t channels) noexcept
    : epoch_(epoch), sample_rate_(sample_rate), channels_(channels) {}

bool CaptureFifo::push(std::span<const float> samples, std::uint64_t host_time, bool valid, bool discontinuity) noexcept {
  const auto frames = channels_ ? samples.size() / channels_ : 0;
  if (exhausted_.load(std::memory_order_relaxed) || frames > std::numeric_limits<std::uint64_t>::max() - next_frame_) {
    exhausted_.store(true, std::memory_order_relaxed); discontinuity_ = true; return false;
  }
  const auto first = next_frame_;
  next_frame_ += frames;
  const auto write = write_.load(std::memory_order_relaxed);
  const auto next = (write + 1) % slots;
  if (!sample_rate_ || !channels_ || samples.empty() || samples.size() % channels_ || samples.size() > CaptureBlock::max_samples || next == read_.load(std::memory_order_acquire)) {
    const auto old = dropped_.load(std::memory_order_relaxed);
    dropped_.store(frames > std::numeric_limits<std::uint64_t>::max() - old ? std::numeric_limits<std::uint64_t>::max() : old + frames, std::memory_order_relaxed);
    discontinuity_ = true;
    return false;
  }
  auto& out = blocks_[write];
  out.epoch = epoch_; out.first_frame = first; out.host_time = host_time;
  out.sample_rate = sample_rate_; out.channels = channels_; out.sample_count = static_cast<std::uint32_t>(samples.size());
  out.host_time_valid = valid; out.discontinuity = discontinuity || discontinuity_;
  std::copy(samples.begin(), samples.end(), out.samples.begin());
  discontinuity_ = false;
  write_.store(next, std::memory_order_release);
  return true;
}

bool CaptureFifo::pop(CaptureBlock& out) noexcept {
  const auto read = read_.load(std::memory_order_relaxed);
  if (read == write_.load(std::memory_order_acquire)) return false;
  out = blocks_[read];
  read_.store((read + 1) % slots, std::memory_order_release);
  return true;
}
std::uint64_t CaptureFifo::dropped_frames() const noexcept { return dropped_.load(std::memory_order_relaxed); }
bool CaptureFifo::exhausted() const noexcept { return exhausted_.load(std::memory_order_relaxed); }
}
