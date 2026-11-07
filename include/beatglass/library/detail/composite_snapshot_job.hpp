#pragma once
#include "beatglass/core/music_collector.hpp"
#include <filesystem>
#include <functional>
#include <optional>
#include <stop_token>
#include <string>
namespace beatglass::library_detail {
/** Terminal outcome of the unverified composite snapshot worker. */
enum class CompositeJobStatus { complete, cancelled, stale_input, error };
/** Worker-owned result; value is present only after successful source verification. */
struct CompositeJobResult {
  CompositeJobStatus status{CompositeJobStatus::error}; ///< Terminal outcome.
  std::optional<UnverifiedMusicAnalysis> value; ///< No cache identity or reusable entry.
  std::string diagnostic; ///< Optional error details.
};
/** Analyze a private snapshot on the calling worker without any cache access.
 * Opens one snapshot, decodes only its borrowed bytes, then destroys the decoder
 * before revalidating snapshot and original. The returned result remains unverified
 * with respect to dependency identity; no store lookup or publication occurs.
 * @param path Original audio path, never modified by this function.
 * @param epoch Nonzero coordinator-issued session epoch.
 * @param limits Hard limits on materialized events and label lengths.
 * @param stop Cancellation checked during snapshot, decode and verification.
 * @param progress Optional synchronous worker callback after each decoded block.
 * Blocking I/O and a callback already in progress cannot be interrupted. A file
 * change after verification remains possible. Not thread-safe or GUI/callback work.
 * Essentia construction must be serialized externally until factory qualification.
 */
CompositeJobResult analyze_snapshot_music(const std::filesystem::path& path, std::uint64_t epoch,
  MusicCollectorLimits limits, std::stop_token stop = {}, const std::function<void(std::uint64_t)>& progress = {});
}
