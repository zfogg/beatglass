#pragma once
#include <cstdint>
#include <limits>

namespace beatglass::windows {
/** Capture-thread-only continuity check for WASAPI device frame positions. The first valid packet establishes a baseline; a valid jump, invalid timestamp or arithmetic overflow requires reacquisition. After invalid data the next valid packet establishes a fresh baseline. This helper neither assigns epochs nor detects missing audio before its first packet. */
class DevicePositionContinuity {
public:
  /** Observe one packet's first device frame and frame count. valid must be false when WASAPI reports a timestamp error. Returns true if this packet must be marked discontinuous. */
  bool observe(std::uint64_t first, std::uint64_t frames, bool valid) noexcept {
    if (!valid || first > std::numeric_limits<std::uint64_t>::max() - frames) {
      have_expected_ = false;
      return true;
    }
    const bool discontinuity = have_expected_ && first != expected_;
    expected_ = first + frames;
    have_expected_ = true;
    return discontinuity;
  }
private:
  std::uint64_t expected_{};
  bool have_expected_{};
};
}
