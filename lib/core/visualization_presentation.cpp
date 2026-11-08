#include "beatglass/core/visualization_presentation.hpp"

namespace beatglass {
void VisualizationPresentation::clear() noexcept {
  frame_.reset();
  gate_.clear();
}

bool VisualizationPresentation::accept(std::uint64_t generation,
                                       std::uint64_t revision,
                                       const VisualizationFrame& frame) noexcept {
  if (!gate_.accept(generation, revision)) return false;
  frame_ = frame;
  return true;
}
}
