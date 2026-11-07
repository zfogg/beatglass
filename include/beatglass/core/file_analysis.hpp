#pragma once
#include "beatglass/core/results.hpp"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
namespace beatglass {
/// Decoded file properties; no reference to the open file is retained.
struct FileMetadata {
  std::uint32_t sample_rate{}; ///< Frames per second.
  std::uint32_t channels{}; ///< Number of interleaved channels.
  std::uint64_t frames{}; ///< Number of frames reported by the decoder.
  std::string format; ///< Decoder container label.
};
/// Snapshot of preliminary file tempo analysis, owned by the caller.
struct TempoResult {
  FileMetadata metadata; ///< Source format and duration.
  std::optional<float> bpm; ///< Current uncalibrated estimate, if available.
  unsigned beats{}; ///< Detected beats in the current acquisition.
  TrackingState state{TrackingState::stopped}; ///< Current lifecycle state.
};
/** Read file properties on a worker thread; this may block on filesystem I/O.
 * @param path Local file path; the source file is not modified.
 * @return A value independent of the decoder's lifetime.
 * @throws std::runtime_error if the file cannot be opened or has invalid format.
 */
FileMetadata read_metadata(const std::filesystem::path& path);
/** Analyze a file incrementally on the caller's worker thread, without pacing.
 * @param path Local audio file; the source file is not modified.
 * @return The latest state and uncalibrated tempo estimate.
 * @throws std::exception on decoder, invalid PCM or estimator errors.
 */
TempoResult analyze_file(const std::filesystem::path& path);
/** Observer called synchronously after each fully observed decoded block.
 * The first argument is the exclusive frame position within the file's epoch;
 * the second is borrowed only for the call. Observers must copy retained data.
 */
using ReplayObserver = std::function<void(std::uint64_t observed_frames, const TempoResult&)>;
/** Replay a file causally, faster than real time, on the caller's worker thread.
 * @param path Local audio file; no source tags or content are modified.
 * @param observer Optional synchronous progress observer; it is not retained.
 * @return The final state after all complete decoded blocks.
 * @throws std::exception on decoder, invalid PCM, estimator or observer errors.
 */
TempoResult replay_file(const std::filesystem::path& path, const ReplayObserver& observer);
}
