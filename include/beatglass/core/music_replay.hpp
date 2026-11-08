#pragma once
#include "beatglass/core/causal_replay.hpp"
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <stdexcept>
#include <stop_token>
namespace beatglass {
/** One aubio beat position emitted after the exclusive observed boundary passes it. */
struct CausalMusicBeat {
  /** Coordinator-issued epoch and event frame. */
  std::uint64_t epoch{}, frame{};
  /** Exclusive input frame observed when the beat was emitted. */
  std::uint64_t observed_through{};
  /** Sample rate of this epoch, for converting frame positions to time. */
  std::uint32_t sample_rate{};
  /** Beat strength, absent when the estimator does not provide one. */
  std::optional<float> strength;
};
/** One analysis window, not a chord segment; its interval is half-open. */
struct CausalTonalFrame {
  /** Epoch and half-open chord-analysis window. */
  std::uint64_t epoch{}, first_frame{}, observed_through{};
  /** First frame contributing to the key estimate, when a key is available. */
  std::optional<std::uint64_t> rolling_key_from;
  /** Rolling key and local major/minor chord, if available. */
  std::optional<std::string> rolling_key, local_chord;
  /** Uncalibrated strengths, absent when unavailable; not probabilities. */
  std::optional<float> rolling_key_strength, local_chord_strength;
  /** Sample rate of this epoch, for converting frame positions to time. */
  std::uint32_t sample_rate{};
};
/** Owned experimental tempo and tonal state; strengths are not probabilities. */
struct CausalMusicState {
  /** Causal tempo and source boundaries. */
  CausalTempoState tempo;
  /** Source sample rate, or zero before a source has been opened. */
  std::uint32_t sample_rate{};
  /** Rolling key and local major/minor triad, absent when unavailable. */
  std::optional<std::string> rolling_key, local_chord;
  /** First frame contributing to the current key estimate, if any. */
  std::optional<std::uint64_t> rolling_key_from;
  /** First frame of the most recent chord window, if any. */
  std::optional<std::uint64_t> tonal_from;
  /** Exclusive end of the most recent tonal window, if any. */
  std::optional<std::uint64_t> tonal_through;
  /** Raw strengths, absent when unavailable; not probabilities. */
  std::optional<float> rolling_key_strength, local_chord_strength;
};
/** Experimental worker-owned composition of real aubio and Essentia trackers.
 * This allocates and runs DSP synchronously and is not thread-safe. Never call
 * it on a capture callback or GUI thread. It borrows PCM only during process,
 * bounds internal windows and does not own or pace a source. Callers must clear
 * retained UI state immediately on source interruption, stop or failure. No
 * chord segments, calibrated confidence, or release-quality accuracy is implied.
 * Serialize Essentia tracker creation across workers until its factory is qualified.
 */
class CausalMusicReplay {
public:
  /** Called synchronously on the worker; the callback is not retained. */
  using BeatObserver = std::function<void(const CausalMusicBeat&)>;
  /** Called synchronously for each new tonal window; copy values retained after return. */
  using TonalObserver = std::function<void(const CausalTonalFrame&)>;
  /** Called synchronously after a decoded block; copy retained values. */
  using StateObserver = std::function<void(const CausalMusicState&)>;
  /** Construct an inactive engine; Essentia must be available in this build. */
  CausalMusicReplay();
  /** Destroy after worker calls and callbacks have completed. */
  ~CausalMusicReplay();
  CausalMusicReplay(const CausalMusicReplay&) = delete;
  CausalMusicReplay& operator=(const CausalMusicReplay&) = delete;
  /** Consume borrowed PCM, emit eligible beats, and return an owned state.
   * @param block Source PCM with nonzero coordinator-issued epoch and frame positions.
   * @param observer Optional callback for beats; events are not retained.
   * @param tonal_observer Optional callback for tonal windows; not retained.
   * @throws std::exception on invalid input, callback or estimator failure;
   * discard the object after an exception and clear any published result.
   * Callbacks must not reenter or destroy this engine.
   */
  CausalMusicState process(const AudioBlock& block, const BeatObserver& observer = {},
                           const TonalObserver& tonal_observer = {});
  /** Clear all evidence after stop or interruption on the owning worker.
   * The next accepted source must have a strictly newer coordinator epoch.
   * This does not revoke copies already delivered to other threads.
   */
  void interrupt() noexcept;
  /** Return the latest owned state, initially stopped. */
  CausalMusicState state() const;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
/** File replay was cancelled before decoding finished. */
class MusicReplayCancelled : public std::runtime_error {
public:
  /** Construct the cancellation exception. */
  MusicReplayCancelled() : std::runtime_error("Music replay cancelled") {}
};
/** Decode and replay a local file with the same causal block engine.
 * This is synchronous worker-thread work and does not modify the input file.
 * @param path Local audio file supported by the configured libsndfile build.
 * @param epoch Nonzero coordinator-allocated epoch for this opened source.
 * @param observer Optional beat callback, invoked synchronously and not retained.
 * @param cancel Checked before decoder reads; it cannot interrupt blocking I/O.
 * @param tonal_observer Optional synchronous callback for tonal windows.
 * @param state_observer Optional synchronous callback after each decoded block.
 * @return The latest owned state; an incomplete final hop is not analyzed. The
 *         opened source is fingerprinted before and after replay; a changed
 *         source is rejected instead of returning analysis for an uncertain file.
 * @throws std::invalid_argument for zero epoch before opening the file;
 * MusicReplayCancelled for cancellation; std::exception for I/O,
 * estimator or callback failure. No cached or shared result is published.
 */
CausalMusicState replay_music_file(const std::filesystem::path& path, std::uint64_t epoch,
                                  const CausalMusicReplay::BeatObserver& observer = {},
                                  std::stop_token cancel = {},
                                  const CausalMusicReplay::TonalObserver& tonal_observer = {},
                                  const CausalMusicReplay::StateObserver& state_observer = {});
}
