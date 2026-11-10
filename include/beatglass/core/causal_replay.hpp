#pragma once
#include "beatglass/core/audio.hpp"
#include "beatglass/core/results.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

namespace beatglass {
/** Owned, provisional state of one causal tempo acquisition. */
struct CausalTempoState {
  /** Source epoch of the most recently accepted block. */
  std::uint64_t epoch{};
  /** Exclusive end of all accepted input, including buffered partial hops. */
  std::uint64_t observed_through{};
  /** Exclusive end of complete hops fed to aubio in this epoch. */
  std::uint64_t analyzed_through{};
  /** Positive aubio BPM while tracking, otherwise absent. */
  std::optional<float> bpm;
  /** Detected beats in the current acquisition, not timestamped events. */
  std::uint64_t beats{};
  /** Recent detected beat boundaries in the current epoch, oldest first. */
  std::vector<std::uint64_t> beat_frames;
  /** Current signal and acquisition state. */
  TrackingState state{TrackingState::stopped};
};
/** Borrowed mono hop, valid only during its synchronous worker callback. */
struct CausalMonoHop {
  /** Epoch and first frame of this hop. */
  std::uint64_t epoch{}, first_frame{};
  /** Sample rate in frames per second. */
  std::uint32_t sample_rate{};
  /** True when downstream evidence must be reset before consuming this hop. */
  bool reset{};
  /** Borrowed mono samples; never retain this view. */
  std::span<const float> samples;
};
/** Provisional worker-owned incremental tempo engine shared by file and live callers.
 * The caller serializes access; this class allocates and runs DSP, so it must
 * never run on a capture callback or GUI thread. It consumes borrowed PCM
 * synchronously and stores only owned, bounded partial-hop data. It does not
 * publish UI state, own a source, pace input, or perform cancellation.
 * Gaps, explicit discontinuities and epoch changes discard buffered samples
 * and reacquire; old epochs, overlapping blocks and same-epoch format changes
 * are rejected and fail the engine closed. Epochs are coordinator-issued. An incomplete hop
 * remains buffered and is not analyzed, including at end of a stream.
 */
class CausalTempoReplay {
public:
  /** Called synchronously after each complete mono hop. The block and samples
   * are borrowed until return; reset marks a new acquisition or resume.
   * The state is the tempo lifecycle after that hop. The observer is not retained
   * and must not reenter or destroy the engine. A nested process call is rejected
   * and fails the outer operation even if its exception is caught. Destroying the
   * engine in a callback remains invalid. No clock anchor is implied.
   */
  using HopObserver = std::function<void(const CausalMonoHop&, TrackingState)>;
  /** Construct an inactive engine; the first valid block starts acquisition. */
  CausalTempoReplay();
  /** Release estimator state after worker calls have stopped. */
  ~CausalTempoReplay();
  CausalTempoReplay(const CausalTempoReplay&) = delete;
  CausalTempoReplay& operator=(const CausalTempoReplay&) = delete;
  /** Consume one observed block; samples are borrowed for this call only.
   * @param block Nonempty, finite interleaved PCM with a valid epoch and format.
   * @param observer Optional synchronous worker callback for complete mono hops.
   * @return An owned state; observed and analyzed positions are exclusive.
   * @throws std::exception for malformed PCM, continuity or estimator errors.
   * After any exception the engine is failed and must be discarded.
   */
  CausalTempoState process(const AudioBlock& block, const HopObserver& observer = {});
  /** Return the most recent state by value, initially stopped. */
  CausalTempoState state() const noexcept;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
