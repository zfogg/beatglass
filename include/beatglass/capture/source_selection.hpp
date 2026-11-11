#pragma once
#include <cstdint>
#include <string_view>
namespace beatglass {
/** User-selectable capture source modes. */
enum class CaptureSourceMode { microphone, room_audio, system_output };
/** Stable CLI/UI label for a capture source mode. */
constexpr std::string_view capture_source_label(CaptureSourceMode mode) noexcept {
  switch (mode) { case CaptureSourceMode::microphone: return "Microphone"; case CaptureSourceMode::room_audio: return "Room audio"; default: return "System output"; }
}
/** Source attribution and lifecycle epoch for accepted capture evidence. */
struct CaptureSourceSelection { CaptureSourceMode mode{CaptureSourceMode::system_output}; std::string_view device_id{}; std::uint64_t epoch{}; bool permission_granted{}; bool discontinuity{}; };
/** Validate a user-facing source mode name. */
constexpr bool capture_source_mode_name(std::string_view name, CaptureSourceMode& mode) noexcept {
  if (name == "microphone") { mode=CaptureSourceMode::microphone; return true; }
  if (name == "room-audio" || name == "room_audio") { mode=CaptureSourceMode::room_audio; return true; }
  if (name == "system-output" || name == "system_output") { mode=CaptureSourceMode::system_output; return true; }
  return false;
}
}
