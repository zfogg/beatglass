#pragma once
#include "windows_loopback_session.hpp"
#include "beatglass/core/tempo_tracker.hpp"
#include <chrono>
#include <memory>
#include <optional>
#include <vector>

namespace beatglass::windows {
/** Experimental worker-owned WASAPI-to-aubio tempo path. This is not a UI model and does not estimate key or chords. Every method runs on one analysis/control thread. */
class LiveTempo {
public:
  /** Borrow the process epoch allocator, which must outlive this object. */
  explicit LiveTempo(SourceEpochAllocator& epochs);
  // Documented manually in the API reference due to Hawkmoth destructor parsing.
  ~LiveTempo();
  /** Request capture of a selected endpoint. True means requested, not running. Stop before retrying a failed attempt. */
  bool start(const std::string& endpoint);
  /** Drain pending capture, update the real tempo tracker, and clear stale evidence on silence, interruption, loss, format changes or errors. Call regularly on the owning worker. Returns false on a source or analysis error. */
  bool pump();
  /** Return the latest tempo in BPM, or empty while stopped, silent, stale or acquiring. The caller must not retain a displayed value across invalidation. */
  std::optional<float> bpm() const;
  /** Return the source lifecycle state on the owning thread. */
  SourceState state() const;
  /** Clear tempo and stop/join capture on the owning thread. */
  void stop();
private:
  void clear() noexcept;
  std::unique_ptr<TempoTracker> tracker_;
  std::vector<float> mono_;
  std::optional<float> bpm_;
  std::uint32_t rate_{}, channels_{};
  std::uint64_t silent_frames_{};
  std::chrono::steady_clock::time_point last_audio_{};
  bool analysis_error_{};
  LoopbackSession session_;
};
}
