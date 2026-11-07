#pragma once

namespace beatglass::windows {
/** GUI-independent state used to decide whether the overlay may enter click-through mode. */
enum class OverlayControlState { unavailable, hidden, minimized, visible };

/** Return whether click-through is safe for the requested overlay/control state. */
constexpr bool canEnableClickThrough(bool overlay_available, bool same_window,
                                     OverlayControlState control_state) noexcept {
  return overlay_available && !same_window && control_state == OverlayControlState::visible;
}
}
