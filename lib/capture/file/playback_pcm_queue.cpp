#include "beatglass/capture/playback_pcm_queue.hpp"
#include <algorithm>
#include <stdexcept>
#include <limits>
namespace beatglass {
namespace { std::size_t checked_capacity(std::size_t slots) { if (!slots || slots == std::numeric_limits<std::size_t>::max()) throw std::invalid_argument("Playback queue dimensions are too large"); return slots + 1; } }
PlaybackPcmQueue::PlaybackPcmQueue(std::size_t slot_count, std::size_t samples_per_slot)
    : capacity_(checked_capacity(slot_count)), samples_per_slot_(samples_per_slot), slots_(std::make_unique<Slot[]>(capacity_)) {
  if (!slot_count || !samples_per_slot) throw std::invalid_argument("Playback queue dimensions must be nonzero");
  for (std::size_t i = 0; i < capacity_; ++i) slots_[i].samples = std::make_unique<float[]>(samples_per_slot_);
}
bool PlaybackPcmQueue::push(const AudioBlock& block) noexcept {
  if (!block.epoch || block.samples.empty() || block.samples.size() > samples_per_slot_) return false;
  const auto w = write_.load(std::memory_order_relaxed), next = (w + 1) % capacity_;
  if (next == read_.load(std::memory_order_acquire)) return false;
  auto& s = slots_[w]; std::copy(block.samples.begin(), block.samples.end(), s.samples.get());
  s.format=block.format; s.epoch=block.epoch; s.first_frame=block.first_frame; s.monotonic_ns=block.monotonic_ns; s.discontinuity=block.discontinuity; s.count=block.samples.size();
  write_.store(next, std::memory_order_release); return true;
}
bool PlaybackPcmQueue::pop(std::span<float> destination, AudioBlock& block) noexcept {
  const auto r=read_.load(std::memory_order_relaxed); if (r==write_.load(std::memory_order_acquire)) return false;
  auto& s=slots_[r]; if (destination.size()<s.count) return false;
  std::copy_n(s.samples.get(), s.count, destination.data()); block={s.format,s.epoch,s.first_frame,s.monotonic_ns,s.discontinuity,std::span<const float>(destination.data(),s.count)};
  read_.store((r+1)%capacity_, std::memory_order_release); return true;
}
std::size_t PlaybackPcmQueue::size() const noexcept { const auto w=write_.load(std::memory_order_acquire),r=read_.load(std::memory_order_acquire); return (w+capacity_-r)%capacity_; }
}
