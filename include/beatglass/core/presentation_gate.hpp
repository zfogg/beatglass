#pragma once
#include "beatglass/core/presentation.hpp"
#include <cstdint>
#include <utility>
namespace beatglass {
/** A sequenced presentation update; an empty snapshot invalidates the display. */
struct PresentationUpdate {
  /** Strictly increasing revision assigned by the single producer. */
  std::uint64_t revision{};
  /** Immutable value or empty to clear the current display. */
  PublishedSnapshot snapshot;
};
/**
 * Presentation-thread filter for delayed or reordered update delivery.
 *
 * The producer assigns a strictly increasing revision to every snapshot and
 * invalidation in production order, and must stop before revision overflow.
 * It transfers immutable updates to the presentation thread through an
 * appropriate queue. The presentation thread owns this object; no method is
 * thread-safe. Recreating it while old updates are queued requires an
 * external session guard or a queue drain. An invalidation only takes effect
 * after it is delivered; this filter cannot cancel already rendered pixels.
 * Readers holding earlier shared copies must not treat them as current.
 */
class PresentationGate {
public:
  /** Apply a newer update. Duplicate or older revisions leave state unchanged. */
  bool apply(PresentationUpdate update) noexcept {
    if (seen_ && update.revision <= revision_) return false;
    seen_ = true; revision_ = update.revision; current_ = std::move(update.snapshot); return true;
  }
  /** Current immutable snapshot, or empty before publication or after clear. */
  PublishedSnapshot current() const noexcept { return current_; }
private:
  bool seen_{};
  std::uint64_t revision_{};
  PublishedSnapshot current_;
};
}
