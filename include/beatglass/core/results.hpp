#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace beatglass {
enum class TrackingState { stopped, silent, acquiring, tracking, reacquiring, error };
struct EventTime { std::uint64_t epoch{}, frame{}; };
// Scores are uncalibrated algorithm strengths, not probabilities.
struct Candidate { std::string label; float score{}; };
struct Beat { EventTime time; float strength{}; };
struct ChordSegment { EventTime start, end; Candidate chord; };
struct Snapshot {
  TrackingState state{TrackingState::stopped};
  EventTime observed_through{};
  std::int64_t publication_monotonic_ns{};
  std::uint64_t analysis_delay_frames{};
  std::optional<float> bpm;
  std::vector<Candidate> tempo_candidates, key_candidates;
  std::vector<Beat> beats;
  std::vector<ChordSegment> chords;
  std::string estimator_version, configuration_id;
};
}
