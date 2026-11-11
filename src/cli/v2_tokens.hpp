#pragma once

namespace beatglass::cli::v2_tokens {
/** Stable v2 semantic color names for ANSI-capable presentation.
 * The CLI must pair these hints with text labels and may omit them when color
 * is unavailable. Values are CSS/terminal-compatible hex strings.
 */
inline constexpr const char* accent = "#7af0d0";
inline constexpr const char* energy = "#77d9ff";
inline constexpr const char* harmonic = "#c7a1ff";
inline constexpr const char* peak = "#ff8fca";
inline constexpr const char* warning = "#ffd27d";
inline constexpr const char* danger = "#ff9a8e";
}
