#pragma once
#include "beatglass/core/music_replay.hpp"
#include <cstdint>
#include <optional>
#include <utility>
namespace beatglass::windows {
/** Owned bounded provisional update for one bridge session. Empty state clears.
 * Revision orders updates within a session; generation identifies invalidations.
 * Labels describe a rolling key and a local window, not a track key or segment.
 */
struct MusicUiUpdate {
  std::uint64_t session{}, generation{}, revision{}; ///< Producer ordering tokens.
  std::optional<CausalMusicState> state; ///< Owned latest state, or a clear.
};
/** GUI-thread gate for reordered updates from a single bridge. A newer session
 * supersedes earlier sessions; within a session revisions must strictly increase
 * and generations must never decrease. A clear takes effect when delivered.
 * Resetting this gate requires draining old deliveries or a new external guard.
 */
class MusicUiGate {
public:
  /** Apply a newer update or clear; reject stale, duplicate, or malformed tokens. */
  bool apply(MusicUiUpdate update) {
    if (!update.session || (seen_ && (update.session<session_ ||
        (update.session==session_ && (update.revision<=revision_ || update.generation<generation_))))) return false;
    seen_=true; session_=update.session; generation_=update.generation; revision_=update.revision;
    state_=std::move(update.state); return true;
  }
  /** Borrow the current state until the next call to apply. */
  const std::optional<CausalMusicState>& current() const noexcept { return state_; }
private:
  bool seen_{};
  std::uint64_t session_{}, generation_{}, revision_{};
  std::optional<CausalMusicState> state_;
};
}
