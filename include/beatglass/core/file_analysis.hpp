#pragma once
#include "beatglass/core/results.hpp"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <stop_token>
namespace beatglass {
/** Decoded file properties; no reference to the open file is retained. */
struct FileMetadata {
  /** Frames per second. */
  std::uint32_t sample_rate{};
  /** Number of interleaved channels. */
  std::uint32_t channels{};
  /** Number of frames reported by the decoder. */
  std::uint64_t frames{};
  /** Decoder container label. */
  std::string format;
};
/** Snapshot of preliminary file tempo analysis, owned by the caller. */
struct TempoResult {
  /** Source format and duration. */
  FileMetadata metadata;
  /** Current uncalibrated estimate, if available. */
  std::optional<float> bpm;
  /** Detected beats in the current acquisition. */
  unsigned beats{};
  /** Current lifecycle state. */
  TrackingState state{TrackingState::stopped};
};
/** One causal beat event from a replay epoch. */
struct ReplayBeat {
  std::uint64_t epoch{};
  std::uint64_t frame{};
  std::uint64_t observed_through{};
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
/** Synchronous observer for causal beat events; the callback is not retained. */
using ReplayBeatObserver = std::function<void(const ReplayBeat&)>;
/** Replay a file causally, faster than real time, on the caller's worker thread.
 * @param path Local audio file; no source tags or content are modified.
 * @param observer Optional synchronous progress observer; it is not retained.
 * @param cancel Optional cooperative cancellation token. Cancellation returns
 * the latest state after complete decoded blocks and never publishes a
 * partial block.
 * @return The final state after all complete decoded blocks.
 * @throws std::exception on decoder, invalid PCM, estimator or observer errors.
 */
TempoResult replay_file(const std::filesystem::path& path, const ReplayObserver& observer,
                        const ReplayBeatObserver& beat_observer = {},
                        std::stop_token cancel = {});
}
