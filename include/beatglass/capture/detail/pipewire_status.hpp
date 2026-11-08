#pragma once

namespace beatglass::pipewire {

/** Provisional private diagnostic values emitted by the PipeWire monitor. */
enum class Diagnostic : int {
  startup_timeout = 12,
  watched_object_invalid = 13,
};

/** Return the numeric value used by the provisional monitor diagnostics. */
constexpr int diagnostic_code(Diagnostic value) noexcept { return static_cast<int>(value); }

} // namespace beatglass::pipewire
