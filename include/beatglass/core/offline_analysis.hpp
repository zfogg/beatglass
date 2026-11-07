#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace beatglass {
/** Offline analysis mode, part of cache identity; refined may use future audio. */
enum class OfflineMode { causal, refined };
/** Named algorithm and version; actual dependency/configuration policy is provisional. */
struct OfflineAlgorithmVersion {
  std::string algorithm; ///< Algorithm or component identifier.
  std::string version; ///< Version sufficient to invalidate changed behavior.
};
/** Owned label with optional uncalibrated strength, never a probability. */
struct OfflineEstimate {
  std::string label; ///< Nonempty label; absence is represented by an optional estimate.
  std::optional<float> strength; ///< Unknown when absent.
};
/** Beat frame and its exclusive observed-at-emission boundary within the result epoch. */
struct OfflineBeat {
  std::uint64_t frame{}; ///< Beat event frame.
  std::uint64_t observed_through{}; ///< Exclusive boundary observed when emitted.
  std::optional<float> strength; ///< Unknown when absent; not a probability.
};
/** Local analysis evidence, not a chord segment; windows may overlap.
 * Both the chord window and rolling-key evidence end at end_frame, exclusively.
 * A gap between windows is unknown, including silence; do not extend a label.
 */
struct OfflineTonalWindow {
  std::uint64_t start_frame{}, end_frame{}; ///< Nonempty half-open local window.
  std::optional<std::uint64_t> rolling_key_from; ///< Rolling key evidence start, if key exists.
  std::optional<OfflineEstimate> rolling_key; ///< Recent evidence, not track-level key.
  std::optional<OfflineEstimate> local_chord; ///< Local candidate, not a segment.
};
/** Owned provisional offline value; no decoder or callback memory is retained.
 * All positions count frames in this result's epoch; sample_rate converts them
 * to time. The epoch is process-session scoped: persistence must rebind or
 * otherwise distinguish it across sessions. observed_through is exclusive and
 * may include a final partial hop beyond analyzed_through. Empty lists and
 * absent estimates mean unknown; gaps do not inherit preceding labels.
 * validated_track_key must only be populated by a separately validated
 * track-level estimator; a final rolling window is insufficient. Producers
 * own vectors and strings; synchronize publication and never use on an audio
 * callback. This value does not implement persistence, paging or cache identity:
 * storage must key validated content, mode, algorithm versions and the full
 * configuration (including profile, windows, hop and downmix), bound materialized
 * event counts and offer paging for long files. No stable wire format is implied.
 * Version and configuration identifiers must reflect the actual run; when
 * they cannot be established, do not invent them or reuse a cached result.
 */
struct OfflineAnalysis {
  std::uint64_t epoch{}; ///< Coordinator-issued epoch for this analysis session.
  std::uint32_t sample_rate{}; ///< Source frames per second.
  std::uint64_t observed_through{}, analyzed_through{}; ///< Exclusive boundaries.
  OfflineMode mode{OfflineMode::causal}; ///< Causal or refined analysis.
  std::string engine_version, configuration_id; ///< Version and full configuration identity.
  std::vector<OfflineAlgorithmVersion> algorithms; ///< Per-component versions.
  std::optional<float> bpm; ///< Tempo estimate, absent when unknown.
  std::optional<OfflineEstimate> validated_track_key; ///< Separately validated track-wide key only.
  std::vector<OfflineBeat> beats; ///< Ordered by event frame.
  std::vector<OfflineTonalWindow> tonal_windows; ///< Ordered by exclusive end; may overlap.
};
/** Caller-selected maximum counts and bytes per text field; no global policy is implied. */
struct OfflineLimits { std::size_t max_beats{}, max_tonal_windows{}, max_algorithms{}, max_text_bytes{}; };
/** Structural validation outcome; no musical accuracy or cache identity is established. */
enum class OfflineValidation { valid, invalid_metadata, limit_exceeded, invalid_beat, invalid_window };
/** Validate metadata, event bounds/order and caller-selected limits without allocation.
 * Must run off the capture callback. This stateless operation is safe for
 * concurrent calls on immutable inputs. Invalid values do not mutate the input.
 * A valid result is not proof of source identity, calibrated scores or successful
 * persistence. Storage must separately validate identity and publication.
 */
OfflineValidation validate_offline_analysis(const OfflineAnalysis& result, OfflineLimits limits) noexcept;
}
