#pragma once
#include <cstdint>
#include <limits>
#include <optional>
namespace beatglass {
struct AudioClockAnchor { std::uint64_t epoch{}, frame{}; std::int64_t monotonic_ns{}; };
// Maps an event in the same epoch to the source monotonic clock. Integer
// arithmetic rounds the magnitude of the offset toward zero. An anchor is a
// point estimate; it does not compensate for clock drift or capture latency.
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
