#include "beatglass/core/visualization_presentation.hpp"
#include <cmath>

namespace beatglass {
void VisualizationPresentation::clear() noexcept {
  frame_.reset();
  gate_.clear();
}

bool VisualizationPresentation::accept(std::uint64_t generation,
                                       std::uint64_t revision,
                                       const VisualizationFrame& frame) noexcept {
  if (!frame.epoch || !frame.sample_rate || frame.end_frame <= frame.first_frame)
    return false;
  for (float value : frame.minimum)
    if (!std::isfinite(value)) return false;
  for (float value : frame.maximum)
    if (!std::isfinite(value)) return false;
  for (float value : frame.magnitude)
    if (!std::isfinite(value) || value < 0.0f) return false;
  if (!gate_.accept(generation, revision)) return false;
  frame_ = frame;
  return true;
}
}
