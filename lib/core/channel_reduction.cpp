#include "beatglass/core/channel_reduction.hpp"
#include <cmath>
namespace beatglass {
bool reduce_loudest_channel(std::span<const float> input, std::size_t channels, std::span<float> output) noexcept {
  if (!channels || input.size() % channels || input.size() / channels != output.size()) return false;
  std::size_t selected = 0; double best = -1;
  for (std::size_t c = 0; c < channels; ++c) {
    double energy = 0;
    for (std::size_t i = 0; i < output.size(); ++i) {
      const float sample = input[i * channels + c];
      if (!std::isfinite(sample)) return false;
      energy += static_cast<double>(sample) * sample;
    }
    if (energy > best) { best = energy; selected = c; }
  }
  for (std::size_t i = 0; i < output.size(); ++i) output[i] = input[i * channels + selected];
  return true;
}
}
