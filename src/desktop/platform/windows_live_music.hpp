#pragma once
#include "windows_loopback_session.hpp"
#include "beatglass/core/music_replay.hpp"
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

namespace beatglass::windows {
/** Provisional worker-owned live composite consumer. Never call from the capture
 * callback or GUI thread. The caller serializes all methods and delivers clears
 * to presentation consumers when generation changes and rejects out-of-order
 * deliveries using revision. Rolling key and local chord
 * are provisional windows, not track key or chord segments. No cache is used.
 * Serialize Essentia construction across workers until its factory is qualified.
 */
class LiveMusic {
public:
  /** Borrow the process allocator, which must outlive this object. */
  explicit LiveMusic(SourceEpochAllocator& epochs);
  /** Take ownership of a backend for deterministic worker tests. */
  LiveMusic(SourceEpochAllocator& epochs, std::unique_ptr<Loopback> backend);
  /** Test seam: clock returns current QPC time in 100 ns units, or empty on failure; afterProcess runs on the worker after DSP and before validation. Neither callback is retained beyond this object. */
  LiveMusic(SourceEpochAllocator& epochs, std::unique_ptr<Loopback> backend,
            std::function<std::optional<std::uint64_t>()> clock, std::function<void()> afterProcess = {});
  /** Stop and join capture on the owning worker. */
  ~LiveMusic();
  /** Start the selected endpoint with a fresh epoch; true means requested. */
  bool start(const std::string& endpoint);
  /** Start using a shared one-attempt authorization; cancellation is thread-safe, while this method remains worker-only. An already executing native call cannot be preempted. Cancellation can race a pump; synchronized generation or authorization gating is required before publishing its output. */
  bool start(const std::string& endpoint, std::shared_ptr<LoopbackStartAuthorization> authorization);
  /** Drain at most 16 packets and run DSP on this worker. On failure clears
   * evidence, stops capture, and returns false. Untimed, future or over-one-second-old capture is discarded; a failed clock fails the attempt. Poll regularly to detect loss. */
  bool pump();
  /** Clear evidence and stop/join capture. */
  void stop();
  /** Borrow the latest provisional state until the next worker call; absent after
   * invalidation or before PCM. Copy with its generation for cross-thread delivery. */
  const std::optional<CausalMusicState>& latest() const noexcept { return latest_; }
  /** Monotonic clear token; consumers must reject results from older tokens. */
  std::uint64_t generation() const noexcept { return generation_; }
  /** Monotonic revision for clears and updates; reject older delivered revisions. */
  std::uint64_t revision() const noexcept { return revision_; }
  /** Current source lifecycle state on the owning worker. */
  SourceState state() const { return session_.state(); }
private:
  void invalidate_publication() noexcept;
  void clear() noexcept;
  std::function<std::optional<std::uint64_t>()> clock_;
  std::function<void()> after_process_;
  std::unique_ptr<CausalMusicReplay> replay_;
  std::optional<CausalMusicState> latest_;
  std::uint64_t generation_{}, revision_{};
  bool failed_{}, exhausted_{};
  std::chrono::steady_clock::time_point last_audio_{};
  LoopbackSession session_;
};
}
