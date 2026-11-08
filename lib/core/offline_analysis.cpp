#include "beatglass/core/offline_analysis.hpp"
#include <cmath>
namespace beatglass {
namespace {
bool valid_estimate(const std::optional<OfflineEstimate>& e) noexcept {
  return !e || (!e->label.empty() && (!e->strength || std::isfinite(*e->strength)));
}
}
OfflineValidation validate_offline_analysis(const OfflineAnalysis& r, OfflineLimits limits) noexcept {
  if (!r.epoch || !r.sample_rate || r.analyzed_through > r.observed_through || r.engine_version.empty() ||
      r.configuration_id.empty() || r.algorithms.empty() || (r.mode != OfflineMode::causal && r.mode != OfflineMode::refined) ||
      (r.bpm && (!std::isfinite(*r.bpm) || *r.bpm <= 0)) || !valid_estimate(r.validated_track_key))
    return OfflineValidation::invalid_metadata;
  if (r.beats.size() > limits.max_beats || r.tonal_windows.size() > limits.max_tonal_windows ||
      r.algorithms.size() > limits.max_algorithms) return OfflineValidation::limit_exceeded;
  if (r.engine_version.size() > limits.max_text_bytes || r.configuration_id.size() > limits.max_text_bytes ||
      (r.validated_track_key && r.validated_track_key->label.size() > limits.max_text_bytes)) return OfflineValidation::limit_exceeded;
  for (const auto& a : r.algorithms) {
    if (a.algorithm.empty() || a.version.empty()) return OfflineValidation::invalid_metadata;
    if (a.algorithm.size() > limits.max_text_bytes || a.version.size() > limits.max_text_bytes) return OfflineValidation::limit_exceeded;
  }
  std::uint64_t previous = 0, previous_observed = 0; bool first = true;
  for (const auto& b : r.beats) {
    if (b.frame >= b.observed_through || b.observed_through > r.observed_through ||
        (!first && (b.frame < previous || b.observed_through < previous_observed)) ||
        (b.strength && !std::isfinite(*b.strength))) return OfflineValidation::invalid_beat;
    previous = b.frame; previous_observed = b.observed_through; first = false;
  }
  previous = 0; first = true;
  for (const auto& w : r.tonal_windows) {
    if ((w.rolling_key && w.rolling_key->label.size() > limits.max_text_bytes) ||
        (w.local_chord && w.local_chord->label.size() > limits.max_text_bytes)) return OfflineValidation::limit_exceeded;
    if (w.start_frame >= w.end_frame || w.end_frame > r.analyzed_through ||
        (!first && w.end_frame < previous) || w.rolling_key.has_value() != w.rolling_key_from.has_value() ||
        (w.rolling_key_from && *w.rolling_key_from >= w.end_frame) ||
        !valid_estimate(w.rolling_key) || !valid_estimate(w.local_chord)) return OfflineValidation::invalid_window;
    previous = w.end_frame; first = false;
  }
  return OfflineValidation::valid;
}
}
