#pragma once
#include <cstddef>
#include <span>
namespace beatglass {
// Worker-thread normalization boundary. Input is interleaved float PCM and
// output is caller-owned. Select the channel with greatest sum-square energy
// in this block (lowest index wins ties). No resampling or clipping is applied.
// Returns false for malformed input without modifying output.
bool reduce_loudest_channel(std::span<const float> input, std::size_t channels, std::span<float> output) noexcept;
}
