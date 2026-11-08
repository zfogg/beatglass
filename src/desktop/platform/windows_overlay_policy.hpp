#pragma once

namespace beatglass::windows {
/** GUI-independent state used to decide whether the overlay may enter click-through mode. */
enum class OverlayControlState { unavailable, hidden, minimized, visible };

/** Return whether click-through is safe for the requested overlay/control state. */
constexpr bool canEnableClickThrough(bool overlay_available, bool same_window,
                                     OverlayControlState control_state) noexcept {
  return overlay_available && !same_window && control_state == OverlayControlState::visible;
}

/** Return whether a user drag may start for the current overlay state. */
constexpr bool canBeginDrag(bool overlay_available, bool overlay_visible,
                            bool click_through) noexcept {
  return overlay_available && overlay_visible && !click_through;
}
}
