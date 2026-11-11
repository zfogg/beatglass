#include "beatglass/library/source_detail.hpp"

#include <algorithm>

namespace beatglass::library {

bool source_detail_has_current_analysis(const SourceDetail& detail) noexcept {
  return detail.availability == SourceAvailability::available &&
         detail.analysis == SourceAnalysisState::complete && detail.freshness_known && detail.fresh;
}

double source_detail_progress(const SourceDetail& detail) noexcept {
  if (detail.total_frames == 0) return -1.0;
  return static_cast<double>(std::min(detail.frames_processed, detail.total_frames)) /
         static_cast<double>(detail.total_frames);
}

const char* source_availability_label(SourceAvailability availability) noexcept {
  switch (availability) {
    case SourceAvailability::available: return "available";
    case SourceAvailability::missing: return "missing";
    case SourceAvailability::unreadable: return "unreadable";
  }
  return "unreadable";
}

const char* source_analysis_state_label(SourceAnalysisState state) noexcept {
  switch (state) {
    case SourceAnalysisState::none: return "none";
    case SourceAnalysisState::queued: return "queued";
    case SourceAnalysisState::running: return "running";
    case SourceAnalysisState::complete: return "complete";
    case SourceAnalysisState::failed: return "failed";
    case SourceAnalysisState::cancelled: return "cancelled";
    case SourceAnalysisState::stale: return "stale";
  }
  return "none";
}

} // namespace beatglass::library
