#pragma once
#include <cstdint>
#include <span>

namespace beatglass {
// Position is measured in frames from the beginning of an epoch, not samples.
/** Declared channel arrangement; unknown is the default. */
enum class ChannelLayout { unknown, mono, stereo, multichannel };
/** Audio format: frames per second, interleaved channel count and layout. */
struct AudioFormat {
  std::uint32_t sample_rate{};
  std::uint16_t channels{};
  ChannelLayout layout{ChannelLayout::unknown};
};
/** Borrowed interleaved normalized float PCM; the sample view is valid only during the call. first_frame counts frames from the epoch start. monotonic_ns records monotonic timing; its exact mapping is source-specific. A discontinuity marks a break in continuity. */
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
