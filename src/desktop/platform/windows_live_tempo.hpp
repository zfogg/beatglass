#pragma once
#include "windows_loopback_session.hpp"
#include "beatglass/core/tempo_tracker.hpp"
#include "beatglass/core/results.hpp"
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace beatglass::windows {
/** One selected-source tempo result and its originating token stamp. An empty BPM is a clear and has no stamp. */
struct StampedLiveTempo {
  /** Optional BPM. */
  std::optional<float> bpm;
  /** Exclusive boundary of the evidence. */
  std::optional<EventTime> observed;
  /** Shared source token. */
  std::shared_ptr<const LoopbackLossToken> loss_token;
  /** Original capture stamp, absent on a clear. */
  std::optional<std::uint64_t> loss_stamp;
};

/** Experimental worker-owned WASAPI-to-aubio tempo path. This is not a UI model and does not estimate key or chords. Every method runs on one analysis/control thread. */
class LiveTempo {
public:
  /** Borrow the process epoch allocator, which must outlive this object. */
  explicit LiveTempo(SourceEpochAllocator& epochs);
  /** Test seam: own an injected capture backend, clock in QPC 100 ns units, and optional post-DSP worker hook. The clock returns empty on failure. */
  LiveTempo(SourceEpochAllocator& epochs, std::unique_ptr<Loopback> backend, std::function<std::optional<std::uint64_t>()> clock, std::function<void()> afterProcess = {});
  // Documented manually in the API reference due to Hawkmoth destructor parsing.
  ~LiveTempo();
  /** Request capture of a selected endpoint. True means requested, not running. Stop before retrying a failed attempt. */
  bool start(const std::string& endpoint);
  /** Drain pending capture, update the real tempo tracker, and clear stale evidence on silence, untimed or future capture, capture older than one second, interruption, loss, format changes or errors. Poll after DSP before publication. Call regularly on the owning worker. Returns false on a source or analysis error. */
  bool pump();
  /** Return the latest tempo in BPM, or empty while stopped, silent, stale or acquiring. The caller must not retain a displayed value across invalidation. */
  std::optional<float> bpm() const;
  /** Exclusive frame boundary and epoch of the latest analyzed packet, or empty after invalidation. */
  std::optional<EventTime> observedThrough() const;
  /** Copy BPM, observed boundary, token and originating stamp together on the control worker. GUI delivery still requires its own session and revision gate. */
  StampedLiveTempo stampedTempo() const noexcept;
  /** Share the capture loss token on the control worker; callers may retain it across threads. */
  std::shared_ptr<const LoopbackLossToken> lossToken() const noexcept;
  /** Packet token stamp associated with the current observed result, absent after a clear. Never replace it with a newer token snapshot. */
  std::optional<std::uint64_t> lossStamp() const noexcept;
  /** Monotonic invalidation generation; changes whenever retained evidence is cleared. Exhaustion stops analysis rather than wrapping. */
  std::uint64_t invalidationGeneration() const;
  /** Return the source lifecycle state on the owning thread. */
  SourceState state() const;
  /** Return the most recent native capture error, or zero when unavailable. */
  std::int32_t error_code() const;
  /** Clear tempo and stop/join capture on the owning thread. */
  void stop();
private:
  void clear() noexcept;
  std::function<std::optional<std::uint64_t>()> clock_;
  std::function<void()> after_process_;
  std::unique_ptr<TempoTracker> tracker_;
  std::vector<float> mono_;
  std::optional<float> bpm_;
  std::optional<EventTime> observed_;
  std::optional<std::uint64_t> loss_stamp_;
  std::uint32_t rate_{}, channels_{};
  std::uint64_t silent_frames_{};
  std::uint64_t invalidation_generation_{};
  std::chrono::steady_clock::time_point last_audio_{};
  bool analysis_error_{};
  bool generation_exhausted_{};
  LoopbackSession session_;
};
}
