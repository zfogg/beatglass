#pragma once
#include "beatglass/core/results.hpp"

namespace beatglass::cli {
/** Return a stable text label for a tracking state for the provisional CLI.
 * This pure function owns no state and is safe to call from any thread.
 * Unrecognized enum values return "unknown", never an active state.
 * @param state State to serialize.
 * @return A pointer to a static, null-terminated string; never null.
 */
inline const char* state_label(TrackingState state) noexcept {
  switch (state) {
    case TrackingState::stopped: return "stopped";
    case TrackingState::silent: return "silent";
    case TrackingState::acquiring: return "acquiring";
    case TrackingState::tracking: return "tracking";
    case TrackingState::reacquiring: return "reacquiring";
    case TrackingState::error: return "error";
  }
  return "unknown";
}
}
