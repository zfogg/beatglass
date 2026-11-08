#pragma once

#include "beatglass/core/visualization.hpp"
#include <optional>

namespace beatglass {
/**
 * Presentation-thread owner for one visualization source.
 *
 * The owner serializes clear() and accept() with painting. Frames are copied
 * into this bounded holder only after the shared generation/revision gate
 * accepts them, so delayed worker deliveries cannot resurrect an old display.
 * The analysis worker must continue to use VisualizationProcessor and must
 * never call this class from its capture callback. clear() synchronously
 * removes the painted frame and advances the generation.
 */
class VisualizationPresentation {
public:
  /** Remove the current frame and invalidate all queued deliveries. */
  void clear() noexcept;
  /** Current source generation token to attach to a worker delivery. */
  std::uint64_t generation() const noexcept { return gate_.generation(); }
  /** Accept a newer frame for the current generation. Returns false if stale. */
  bool accept(std::uint64_t generation, std::uint64_t revision,
              const VisualizationFrame& frame) noexcept;
  /** Return the currently painted frame, if any. */
  const std::optional<VisualizationFrame>& current() const noexcept { return frame_; }
  /** Revision of the currently painted frame, or zero after clear. */
  std::uint64_t acceptedRevision() const noexcept { return accepted_revision_; }

private:
  VisualizationPublicationGate gate_;
  std::optional<VisualizationFrame> frame_;
  std::uint64_t accepted_revision_{};
};
}
