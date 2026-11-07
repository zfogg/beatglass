#pragma once
#include <cstdint>
#include <limits>

namespace beatglass::windows {
/** Validate all frame and timestamp additions for one WASAPI buffer before
 * publishing any part of it. Arguments are native 32-bit buffer dimensions;
 * invalid timestamps are ignored. A false result requires releasing the buffer
 * and interrupting the current capture attempt. No allocation or locking. */
inline bool captureBufferArithmeticValid(std::uint64_t first_frame, std::uint32_t frames,
                                         std::uint64_t qpc_100ns, bool timestamp_valid,
                                         std::uint32_t rate, std::uint32_t chunk_limit) noexcept {
  constexpr auto max = std::numeric_limits<std::uint64_t>::max();
  if (!rate || !chunk_limit || first_frame > max - frames) return false;
  if (!timestamp_valid || !frames) return true;
  const std::uint64_t last_offset = (static_cast<std::uint64_t>(frames - 1) / chunk_limit) * chunk_limit;
  const std::uint64_t last_time_offset = last_offset * 10000000ULL / rate;
  return qpc_100ns <= max - last_time_offset;
}

/** Capture-thread-only continuity check for WASAPI device frame positions. The first valid packet establishes a baseline; a valid jump, invalid timestamp or arithmetic overflow requires reacquisition. After invalid data the next valid packet establishes a fresh baseline. This helper neither assigns epochs nor detects missing audio before its first packet. */
class DevicePositionContinuity {
public:
  /** Observe one packet's first device frame and frame count. valid must be false when WASAPI reports a timestamp error. flagged records a native discontinuity. Returns true if this packet must be marked discontinuous; an empty discontinuous packet also marks the next nonempty packet. */
  bool observe(std::uint64_t first, std::uint64_t frames, bool valid, bool flagged = false) noexcept {
    if (!frames && (!valid || flagged)) pending_gap_ = true;
    if (!valid || first > std::numeric_limits<std::uint64_t>::max() - frames) {
      have_expected_ = false;
      return true;
    }
    const bool discontinuity = flagged || pending_gap_ || (have_expected_ && first != expected_);
    if (frames) pending_gap_ = false;
    else if (discontinuity) pending_gap_ = true;
    expected_ = first + frames;
    have_expected_ = true;
    return discontinuity;
  }
private:
  std::uint64_t expected_{};
  bool have_expected_{}, pending_gap_{};
};
}
