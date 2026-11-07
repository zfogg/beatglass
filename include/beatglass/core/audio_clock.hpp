#pragma once
#include <cstdint>
#include <limits>
#include <optional>
namespace beatglass {
/// A point estimate relating one frame in an epoch to a monotonic clock.
struct AudioClockAnchor {
  std::uint64_t epoch{}; ///< Source session identifier.
  std::uint64_t frame{}; ///< Frame position within the epoch.
  std::int64_t monotonic_ns{}; ///< Monotonic clock time in nanoseconds.
};
/**
 * Map a frame in the same epoch to the source monotonic clock.
 *
 * Integer arithmetic rounds the offset magnitude toward zero. The anchor is
 * a point estimate and does not compensate for drift or capture latency.
 * This stateless function is safe to call concurrently.
 * @param anchor Source frame and monotonic-clock anchor.
 * @param epoch Epoch containing the requested frame.
 * @param frame Requested frame position within that epoch.
 * @param rate Source sample rate in frames per second.
 * @return Monotonic nanoseconds, or empty on an epoch mismatch, zero rate,
 *         or arithmetic overflow.
 */
inline std::optional<std::int64_t> frame_to_monotonic_ns(AudioClockAnchor anchor, std::uint64_t epoch,
                                                         std::uint64_t frame, std::uint32_t rate) noexcept {
  if (!rate || epoch != anchor.epoch) return std::nullopt;
  const bool forward = frame >= anchor.frame;
  const auto delta = forward ? frame - anchor.frame : anchor.frame - frame;
  constexpr std::uint64_t billion = 1000000000;
  constexpr auto max = std::numeric_limits<std::int64_t>::max();
  constexpr auto min = std::numeric_limits<std::int64_t>::min();
  const auto seconds = delta / rate;
  if (seconds > static_cast<std::uint64_t>(max) / billion) return std::nullopt;
  const auto ns = seconds * billion + (delta % rate) * billion / rate;
  if (ns > static_cast<std::uint64_t>(max)) return std::nullopt;
  const auto offset = static_cast<std::int64_t>(ns);
  if (forward) { if (anchor.monotonic_ns > max - offset) return std::nullopt; return anchor.monotonic_ns + offset; }
  if (anchor.monotonic_ns < min + offset) return std::nullopt;
  return anchor.monotonic_ns - offset;
}
}
