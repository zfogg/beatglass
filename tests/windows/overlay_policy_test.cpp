#include "windows_overlay_policy.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Overlay click-through requires a visible escape path") {
  using beatglass::windows::OverlayControlState;
  using beatglass::windows::canEnableClickThrough;
  CHECK_FALSE(canEnableClickThrough(false, false, OverlayControlState::visible));
  CHECK_FALSE(canEnableClickThrough(true, true, OverlayControlState::visible));
  CHECK_FALSE(canEnableClickThrough(true, false, OverlayControlState::hidden));
  CHECK_FALSE(canEnableClickThrough(true, false, OverlayControlState::minimized));
  CHECK_FALSE(canEnableClickThrough(true, false, OverlayControlState::unavailable));
  CHECK(canEnableClickThrough(true, false, OverlayControlState::visible));
}

TEST_CASE("Overlay dragging requires a visible non-click-through overlay") {
  using beatglass::windows::canBeginDrag;
  CHECK_FALSE(canBeginDrag(false, true, false));
  CHECK_FALSE(canBeginDrag(true, false, false));
  CHECK_FALSE(canBeginDrag(true, true, true));
  CHECK(canBeginDrag(true, true, false));
}
