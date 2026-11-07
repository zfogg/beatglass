#pragma once
#include "beatglass/core/music_replay.hpp"
#include "beatglass/core/offline_analysis.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
namespace beatglass {
/** Result event count limits and a byte limit per individual label. Zero permits no entries or label bytes. */
struct MusicCollectorLimits {
  std::size_t max_beats{}, max_tonal_windows{}, max_label_bytes{}; ///< Counts per result; bytes per label.
};
/** A complete owned causal result whose dependency identity is not verified.
 * Initial identity fields are empty. This is an advisory provenance marker: value
 * is public and mutable, and filling its fields can satisfy structural validation
 * without verifying provenance. Storage must reject unverified producer results
 * or require a separately validated capability before cache publication. Display
 * requires the caller to verify the source and commit the job separately.
 */
struct UnverifiedMusicAnalysis {
  OfflineAnalysis value; ///< Owned events and windows; no borrowed PCM remains.
  /** Always false; callers must enforce this result at the storage boundary. */
  bool cache_reusable() const noexcept { return false; }
};
/** Bounded worker-only collector for one contiguous source starting at frame zero.
 * Not thread-safe and never callable from a capture callback or GUI thread.
 * DSP and synchronous callbacks may allocate. The caller owns the decoder,
 * source, cancellation and source revalidation. Do not publish before EOF and
 * successful source revalidation. An exception permanently invalidates this
 * object and its staged events; destroy it without calling finish.
 * Serialize Essentia construction across workers until its factory is qualified.
 */
class MusicCollector {
public:
  /** Create a collector with a nonzero coordinator-issued epoch and limits. */
  MusicCollector(std::uint64_t epoch, MusicCollectorLimits limits);
  /** Destroy after all worker calls have completed. */
  ~MusicCollector();
  MusicCollector(const MusicCollector&) = delete;
  MusicCollector& operator=(const MusicCollector&) = delete;
  /** Consume borrowed samples synchronously; block must be contiguous and use
   * the configured epoch. Exceptions, including a limit breach, fail closed.
   */
  void process(const AudioBlock& block);
  /** Move the result out after the caller has observed successful decoder EOF.
   * Requires at least one block; a second finish or process fails. This method
   * does not itself check EOF, cancellation, or source identity.
   */
  UnverifiedMusicAnalysis finish();
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
