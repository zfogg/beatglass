#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace beatglass::library {

/** Availability of the selected source at the time of the snapshot. */
enum class SourceAvailability { available, missing, unreadable };

/** Analysis lifecycle and evidence state for a selected source. */
enum class SourceAnalysisState { none, queued, running, complete, failed, cancelled, stale };

/** Immutable GUI/CLI-ready state for one selected source.
 * This model owns its strings, performs no I/O, and is safe to copy between a
 * worker snapshot and a presentation thread. A complete state is meaningful
 * only when availability is available and freshness is fresh. */
struct SourceDetail {
  std::filesystem::path path;
  std::string source_id;
  std::string title;
  SourceAvailability availability{SourceAvailability::missing};
  SourceAnalysisState analysis{SourceAnalysisState::none};
  std::uint64_t frames_processed{};
  std::uint64_t total_frames{};
  bool freshness_known{};
  bool fresh{};
  std::string diagnostic;
};

/** Return whether analysis evidence may be presented as current data. */
bool source_detail_has_current_analysis(const SourceDetail& detail) noexcept;

/** Clamp a progress value to a known total; zero total means indeterminate. */
double source_detail_progress(const SourceDetail& detail) noexcept;

/** Return a stable presentation label for a source availability value. */
const char* source_availability_label(SourceAvailability availability) noexcept;

/** Return a stable presentation label for an analysis state value. */
const char* source_analysis_state_label(SourceAnalysisState state) noexcept;

} // namespace beatglass::library
