#pragma once
#include <cstdint>
#include <limits>
#include <optional>
namespace beatglass {
/** Identifies the clock used by a live anchor; unspecified must not be compared across sources. */
enum class AudioClockDomain { unspecified, process_monotonic, backend_monotonic };
/** A point estimate relating one frame in an epoch to a clock. Absence is represented by an optional anchor, not a zero time. */
struct AudioClockAnchor {
  std::uint64_t epoch{}; ///< Source session identifier.
  std::uint64_t frame{}; ///< Frame position within the epoch.
  std::int64_t monotonic_ns{}; ///< Monotonic clock time in nanoseconds.
  AudioClockDomain domain{AudioClockDomain::unspecified}; ///< Clock domain of the timestamp.
  std::optional<std::uint64_t> uncertainty_ns; ///< Known uncertainty, absent when unknown.
  std::optional<std::uint64_t> latency_ns; ///< Known capture latency, absent when unknown.
};
/** Map an optional anchor; absence is not interpreted as time zero. */
inline std::optional<std::int64_t> frame_to_monotonic_ns(std::optional<AudioClockAnchor> anchor, std::uint64_t epoch,
                                                         std::uint64_t frame, std::uint32_t rate) noexcept;
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
inline std::optional<std::int64_t> frame_to_monotonic_ns(std::optional<AudioClockAnchor> anchor, std::uint64_t epoch,
                                                         std::uint64_t frame, std::uint32_t rate) noexcept {
  return anchor ? frame_to_monotonic_ns(*anchor, epoch, frame, rate) : std::nullopt;
}
}
