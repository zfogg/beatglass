#pragma once
#include "beatglass/core/results.hpp"
#include <memory>
#include <string>
#include <cmath>
#include <iomanip>
#include <sstream>
namespace beatglass {
/** Format one positive BPM estimate with the same precision used by all adapters. */
inline std::string format_presentation_bpm(float bpm) {
  std::ostringstream value;
  value << std::fixed << std::setprecision(1) << bpm;
  return value.str();
}
// Immutable shared snapshots may be handed to a presentation thread; the
// publisher bounds timeline lengths before publication. GUI types stay local.
/** Shared immutable snapshot; the publisher is responsible for bounding timeline lengths before publication. */
using PublishedSnapshot = std::shared_ptr<const Snapshot>;
/** Application-facing labels and stale flag. */
struct PresentationState { std::string status, tempo, key, chord; std::uint64_t beat_count{}, observed_frames{}, analysis_delay_frames{}; std::optional<float> confidence; std::vector<float> tempo_history; std::vector<std::string> tonal_history; std::vector<std::uint64_t> beat_frames; bool stale{}; };
/**
 * Convert one immutable analysis snapshot to the product's shared display state.
 * This is pure and thread-safe: it borrows no data and performs no UI work.
 * Non-tracking states intentionally clear tempo/key/chord labels.
 */
inline PresentationState present_snapshot(const Snapshot& snapshot) {
  PresentationState result{"Stopped", "—", "—", "—", 0, snapshot.observed_through.frame, snapshot.analysis_delay_frames, std::nullopt, {}, {}, {}, false};
  for (const auto& beat : snapshot.beats) if (beat.time.epoch == snapshot.observed_through.epoch) result.beat_frames.push_back(beat.time.frame);
  if (result.beat_frames.size() > 64) result.beat_frames.erase(result.beat_frames.begin(), result.beat_frames.end() - 64);
  result.beat_count = result.beat_frames.size();
  result.tempo_history = snapshot.tempo_history;
  if (result.tempo_history.size() > 32) result.tempo_history.erase(result.tempo_history.begin(), result.tempo_history.end() - 32);
  for (const auto& chord : snapshot.chords) if (!chord.chord.label.empty()) result.tonal_history.push_back(chord.chord.label);
  if (result.tonal_history.size() > 8) result.tonal_history.erase(result.tonal_history.begin(), result.tonal_history.end() - 8);
  result.stale = snapshot.state != TrackingState::tracking && snapshot.state != TrackingState::stopped;
  switch (snapshot.state) {
    case TrackingState::stopped: result.status = "Stopped"; break;
    case TrackingState::silent: result.status = "Silent"; break;
    case TrackingState::acquiring: result.status = "Acquiring"; break;
    case TrackingState::tracking: result.status = "Tracking"; break;
    case TrackingState::reacquiring: result.status = "Reacquiring"; break;
    case TrackingState::error: result.status = "Error"; break;
  }
  // A reacquiring/silent/error snapshot is an invalidation boundary for every
  // time-varying estimate, including history. Keeping old tempo samples here
  // makes the GUI and TUI appear to track audio that is no longer fresh.
  if (snapshot.state != TrackingState::tracking) {
    result.tempo_history.clear();
    result.tonal_history.clear();
    result.beat_frames.clear();
    result.beat_count = 0;
    return result;
  }
  if (snapshot.confidence && std::isfinite(*snapshot.confidence) && *snapshot.confidence >= 0.0f && *snapshot.confidence <= 1.0f) result.confidence = *snapshot.confidence;
  if (snapshot.bpm && std::isfinite(*snapshot.bpm) && *snapshot.bpm > 0) {
    result.tempo = format_presentation_bpm(*snapshot.bpm);
  }
  if (!snapshot.key_candidates.empty() && !snapshot.key_candidates.front().label.empty())
    result.key = snapshot.key_candidates.front().label;
  if (!snapshot.chords.empty() && !snapshot.chords.back().chord.label.empty())
    result.chord = snapshot.chords.back().chord.label;
  return result;
}
/** Provisional presentation interface; GUI types remain local to the application. */
class PresentationAdapter {
public:
  virtual ~PresentationAdapter() = default;
  /** Convert a borrowed snapshot into presentation labels and stale status. Threading and error behavior depend on the implementation. */
  virtual PresentationState present(const Snapshot&) const = 0;
};
}
