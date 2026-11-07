#include "beatglass/capture/coreaudio_fifo_internal.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace beatglass::coreaudio {
static_assert(std::atomic<std::size_t>::is_always_lock_free && std::atomic<std::uint64_t>::is_always_lock_free && std::atomic<std::uint32_t>::is_always_lock_free && std::atomic<bool>::is_always_lock_free);
namespace { constexpr std::uint32_t closed_bit = std::uint32_t{1} << 31; }
CallbackEntry CallbackGate::try_enter() noexcept {
  std::uint32_t expected = 0;
  if (state_.compare_exchange_strong(expected, 1, std::memory_order_acquire, std::memory_order_acquire)) return CallbackEntry::entered;
  return (expected & closed_bit) ? CallbackEntry::closed : CallbackEntry::busy;
}
void CallbackGate::leave() noexcept { state_.fetch_sub(1, std::memory_order_release); }
void CallbackGate::close() noexcept { state_.fetch_or(closed_bit, std::memory_order_acq_rel); }
bool CallbackGate::drained() const noexcept { return state_.load(std::memory_order_acquire) == closed_bit; }
void CallbackGate::reopen() noexcept { state_.store(0, std::memory_order_relaxed); }
bool cleanup_in_order(void* context, const CleanupOps& ops) noexcept {
  return ops.stop_io(context) && ops.destroy_io(context) && ops.destroy_aggregate(context) && ops.destroy_tap(context);
}

bool NativeTimeline::observe(double sample_time, bool valid, std::uint32_t frames) noexcept {
  valid = valid && std::isfinite(sample_time);
  const bool discontinuity = seen_ && (valid != last_valid_ || (valid && last_valid_ && sample_time != expected_));
  if (valid) expected_ = sample_time + frames;
  seen_ = true;
  last_valid_ = valid;
  return discontinuity;
}
CaptureFifo::CaptureFifo(std::uint64_t epoch, std::uint32_t sample_rate, std::uint32_t channels) noexcept
    : epoch_(epoch), sample_rate_(sample_rate), channels_(channels) {}

bool CaptureFifo::push(std::span<const float> samples, std::uint64_t host_time, bool valid, bool discontinuity,
                       double source_sample_time, bool source_sample_time_valid) noexcept {
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
  out.source_sample_time = source_sample_time; out.source_sample_time_valid = source_sample_time_valid;
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
