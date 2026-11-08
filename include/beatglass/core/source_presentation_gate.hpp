#pragma once

#include "beatglass/core/presentation.hpp"
#include <cstdint>
#include <utility>

namespace beatglass {

/**
 * An immutable presentation delivery tagged with its source lifetime.
 *
 * One control owner must allocate non-zero session and generation values and
 * assign strictly increasing revisions to snapshots and clears. A generation
 * is retired on stop, seek, source change, discontinuity or overflow; queued
 * deliveries from that generation are then rejected. The presentation thread
 * owns the gate and must serialize calls; this type does not provide a queue
 * or revoke a snapshot already painted by a GUI.
 */
struct SourcePresentationUpdate {
  std::uint64_t session{};   ///< Process/session identity; zero is invalid.
  std::uint64_t generation{};///< Source lifetime; zero is invalid for a snapshot.
  std::uint64_t revision{};  ///< Strictly increasing within a session.
  PublishedSnapshot snapshot;///< Empty means clear, including invalidation.
};

/** Reject reordered deliveries across source lifetimes while retaining latest state. */
class SourcePresentationGate {
public:
  /** Apply a valid newer update; return false for malformed or stale input. */
  bool apply(SourcePresentationUpdate update) noexcept {
    if (!update.session || !update.generation || !update.revision ||
        (seen_ && (update.session < session_ ||
                   (update.session == session_ && update.revision <= revision_) ||
                   (update.session == session_ && update.generation < generation_))))
      return false;
    seen_ = true;
    session_ = update.session;
    generation_ = update.generation;
    revision_ = update.revision;
    current_ = std::move(update.snapshot);
    return true;
  }

  /** Return the latest accepted snapshot, or empty after a delivered clear. */
  PublishedSnapshot current() const noexcept { return current_; }

private:
  bool seen_{};
  std::uint64_t session_{}, generation_{}, revision_{};
  PublishedSnapshot current_;
};
}
