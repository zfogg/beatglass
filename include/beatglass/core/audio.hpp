#pragma once
#include <cstdint>
#include <span>

namespace beatglass {
// Position is measured in frames from the beginning of an epoch, not samples.
enum class ChannelLayout { unknown, mono, stereo, multichannel };
struct AudioFormat {
  std::uint32_t sample_rate{};
  std::uint16_t channels{};
  ChannelLayout layout{ChannelLayout::unknown};
};
struct AudioBlock {
  AudioFormat format;
  std::uint64_t epoch{};
  std::uint64_t first_frame{};
  std::int64_t monotonic_ns{};
  bool discontinuity{};
  // Interleaved normalized float PCM. The view is valid only during the call.
  std::span<const float> samples;
};
}
