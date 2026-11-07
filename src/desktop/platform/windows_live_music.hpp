#pragma once
#include "windows_loopback_session.hpp"
#include "beatglass/core/music_replay.hpp"
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

namespace beatglass::windows {
/** Provisional worker-owned live composite consumer. Never call from the capture
 * callback or GUI thread. The caller serializes all methods and delivers clears
 * to presentation consumers when generation changes. Rolling key and local chord
 * are provisional windows, not track key or chord segments. No cache is used.
 * Serialize Essentia construction across workers until its factory is qualified.
 */
class LiveMusic {
public:
  /** Borrow the process allocator, which must outlive this object. */
  explicit LiveMusic(SourceEpochAllocator& epochs);
  /** Take ownership of a backend for deterministic worker tests. */
  LiveMusic(SourceEpochAllocator& epochs, std::unique_ptr<Loopback> backend);
  /** Stop and join capture on the owning worker. */
  ~LiveMusic();
  /** Start the selected endpoint with a fresh epoch; true means requested. */
  bool start(const std::string& endpoint);
  /** Drain at most 16 packets and run DSP on this worker. On failure clears
   * evidence, stops capture, and returns false. Poll regularly to detect loss. */
  bool pump();
  /** Clear evidence and stop/join capture. */
  void stop();
  /** Borrow the latest provisional state until the next worker call; absent after
   * invalidation or before PCM. Copy with its generation for cross-thread delivery. */
  const std::optional<CausalMusicState>& latest() const noexcept { return latest_; }
  /** Monotonic clear token; consumers must reject results from older tokens. */
  std::uint64_t generation() const noexcept { return generation_; }
  /** Current source lifecycle state on the owning worker. */
  SourceState state() const { return session_.state(); }
private:
  void clear() noexcept;
  std::unique_ptr<CausalMusicReplay> replay_;
  std::optional<CausalMusicState> latest_;
  std::uint64_t generation_{};
  bool failed_{}, exhausted_{};
  std::chrono::steady_clock::time_point last_audio_{};
  LoopbackSession session_;
};
}
