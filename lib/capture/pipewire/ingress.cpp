#include "beatglass/capture/detail/pipewire_ingress.hpp"
#include <cstdint>
#include <limits>
namespace beatglass::pipewire {
IngressResult MonitorIngress::ingest(std::span<const std::byte> data, std::size_t offset, std::size_t size,
                                     std::int32_t stride, bool corrupted, bool discontinuity) noexcept {
  constexpr auto frame_bytes = 2 * sizeof(float);
  if (failed_) return IngressResult::invalid;
  if (!epoch_ || corrupted || offset > data.size() || size > data.size() - offset ||
      size % frame_bytes || (stride != 0 && stride != frame_bytes) ||
      (size && (!data.data() || (reinterpret_cast<std::uintptr_t>(data.data()) + offset) % alignof(float))) ||
      size / frame_bytes > std::numeric_limits<std::uint64_t>::max() - next_frame_) {
    failed_ = true;
    return IngressResult::invalid;
  }
  if (!size) return IngressResult::empty;
  const auto* samples = reinterpret_cast<const float*>(data.data() + offset);
  AudioBlock block{{48000, 2, ChannelLayout::stereo}, epoch_, next_frame_, 0, discontinuity,
                   {samples, size / sizeof(float)}};
  next_frame_ += size / frame_bytes;
  return queue_.push(block) ? IngressResult::copied : IngressResult::dropped;
}
}
