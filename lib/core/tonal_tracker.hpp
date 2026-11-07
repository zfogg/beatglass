#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>

namespace beatglass::detail {
/** Provisional worker-local tonal estimate; strengths are not probabilities. */
struct TonalEstimate {
  /** End of observed PCM, exclusive; values use frames in the current epoch. */
  std::uint64_t observed_through{};
  /** Track key from recent HPCP evidence, or absent when unknown. */
  std::optional<std::string> key;
  /** Local major/minor triad, or absent when unknown. */
  std::optional<std::string> chord;
  /** Uncalibrated Essentia key correlation. */
  float key_strength{};
  /** Uncalibrated Essentia chord correlation. */
  float chord_strength{};
};
/** Incremental, unpaced experimental Essentia analyzer, owned by one worker.
 * PCM is borrowed only during process(); internal windows are bounded and owned.
 * A new epoch, a gap or explicit reset discards all previous tonal evidence.
 * Every result uses observed samples only. Not safe for concurrent calls and
 * not for use on an audio callback or as a calibrated production estimator.
 * Essentia is initialized once for process lifetime; create instances serially
 * until concurrent factory use has been qualified.
 */
class TonalTracker {
public:
  /** Create at the source sample rate. Rates below 12 kHz are unsupported.
   * @throws std::exception if the rate is invalid or Essentia initialization fails.
   */
  explicit TonalTracker(std::uint32_t sample_rate);
  /** Destroy on the owning worker, after all calls have stopped. */
  ~TonalTracker();
  TonalTracker(const TonalTracker&) = delete;
  TonalTracker& operator=(const TonalTracker&) = delete;
  /** Consume contiguous mono PCM and retain at most the bounded analysis window.
   * @param samples Finite mono float PCM, borrowed for this call only.
   * @param epoch Source epoch; a change resets prior evidence.
   * @param first Frame position of the first sample within the epoch.
   * @param discontinuity Reset even when epoch and position match.
   * @throws std::exception on invalid samples, position overflow or DSP failure.
   * Invalid PCM, position overflow and DSP failures clear estimates; discard the object after a DSP failure.
   */
  void process(std::span<const float> samples, std::uint64_t epoch, std::uint64_t first, bool discontinuity = false);
  /** Return an owned copy of the last completed analysis window, or unknown. */
  TonalEstimate estimate() const;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
