#include "beatglass/core/visualization_presentation.hpp"
#include <cmath>

namespace beatglass {
void VisualizationPresentation::clear() noexcept {
  frame_.reset();
  accepted_revision_ = 0;
  gate_.clear();
}

bool VisualizationPresentation::accept(std::uint64_t generation,
                                       std::uint64_t revision,
                                       const VisualizationFrame& frame) noexcept {
  if (!frame.epoch || !frame.sample_rate || frame.end_frame <= frame.first_frame)
    return false;
  for (std::size_t i = 0; i < frame.minimum.size(); ++i) {
    const float minimum = frame.minimum[i];
    const float maximum = frame.maximum[i];
    // Presentation must fail closed: malformed extrema otherwise produce an
    // inverted or unbounded waveform even though the publication gate accepts
    // the revision as fresh.
    if (!std::isfinite(minimum) || !std::isfinite(maximum)
        || minimum < -1.0f || maximum > 1.0f || minimum > maximum)
      return false;
  }
  for (float value : frame.magnitude)
    if (!std::isfinite(value) || value < 0.0f) return false;
  if (!gate_.accept(generation, revision)) return false;
  frame_ = frame;
  accepted_revision_ = revision;
  return true;
}
}
