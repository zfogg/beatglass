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
  std::optional<UnverifiedMusicAnalysis> value; ///< Advisory marker; storage must reject unverified results.
  std::string diagnostic; ///< Optional error details.
};
/** Analyze a private snapshot on the calling worker without any cache access.
 * Opens one snapshot, decodes only its borrowed bytes, then destroys the decoder
 * before revalidating snapshot and original. The returned result remains unverified
 * with respect to dependency identity; no store lookup or publication occurs.
 * The mutable result is advisory, not a storage capability. Cache storage must
 * reject unverified producers or require a separately validated capability.
 * @param path Original audio path, never modified by this function.
 * @param epoch Nonzero coordinator-issued session epoch.
 * @param limits Event count limits and byte limit per individual label.
 * @param stop Cancellation checked during snapshot, decode and verification.
 * @param progress Optional synchronous worker callback after each decoded block.
 * Blocking I/O and a callback already in progress cannot be interrupted. A file
 * change after verification remains possible. Not thread-safe or GUI/callback work.
 * Essentia construction must be serialized externally until factory qualification.
 */
CompositeJobResult analyze_snapshot_music(const std::filesystem::path& path, std::uint64_t epoch,
  MusicCollectorLimits limits, std::stop_token stop = {}, const std::function<void(std::uint64_t)>& progress = {});
}
