#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace beatglass {
/** Provisional tracking states; transition rules are not yet implemented here. */
enum class TrackingState { stopped, silent, acquiring, tracking, reacquiring, error };
/** Sample-frame position within an epoch. */
struct EventTime { std::uint64_t epoch{}, frame{}; };
// Scores are uncalibrated algorithm strengths, not probabilities.
/** Label and uncalibrated algorithm strength; the score is not a probability. */
struct Candidate { std::string label; float score{}; };
/** Beat event position and strength. */
struct Beat { EventTime time; float strength{}; };
/** Chord candidate over the nonempty half-open interval [start, end), within one epoch. */
struct ChordSegment { EventTime start, end; Candidate chord; };
/** Value snapshot of provisional analysis results. observed_through is the exclusive audio-frame boundary: observed frames are [0, observed_through), beats precede it, and chord ends may equal it. publication_monotonic_ns is publication time, and analysis_delay_frames records delay in frames. Empty candidates and absent BPM express missing results. No synchronization or history bounds are imposed by this type. */
struct Snapshot {
  TrackingState state{TrackingState::stopped};
  EventTime observed_through{};
  std::int64_t publication_monotonic_ns{};
  std::uint64_t analysis_delay_frames{};
  std::optional<float> bpm;
  /** Uncalibrated signal-health confidence in [0, 1], when available. */
  std::optional<float> confidence;
  std::vector<Candidate> tempo_candidates, key_candidates;
  std::vector<Beat> beats;
  std::vector<ChordSegment> chords;
  std::string estimator_version, configuration_id;
};
}
