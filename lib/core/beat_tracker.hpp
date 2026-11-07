#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
namespace beatglass::detail {
/** Aubio beat event in the source epoch, published only after observation. */
struct BeatEvent {
  /** Epoch supplied with the source PCM. */
  std::uint64_t epoch{};
  /** Aubio's estimated event frame within the epoch; timing accuracy is uncalibrated. */
  std::uint64_t frame{};
  /** Exclusive PCM boundary at delivery; frame is strictly below this. */
  std::uint64_t observed_through{};
};
/** Synchronous worker-owned aubio beat tracker; not safe on capture callbacks.
 * PCM is borrowed only for each call; partial hops are stored internally.
 * An epoch change, gap or explicit discontinuity discards previous evidence.
 * The caller must serialize calls and destroy the object on the owning worker.
 * An event exactly on the current exclusive boundary is held until another
 * sample arrives; if the stream ends there it is not emitted.
 */
class BeatTracker {
public:
  /** Called synchronously on the owning worker; the callback is never retained. */
  using Observer = std::function<void(const BeatEvent&)>;
  /** Create with a positive sample rate; throws on invalid rate or aubio failure. */
  explicit BeatTracker(std::uint32_t sample_rate);
  /** Release aubio state after processing has stopped. */
  ~BeatTracker();
  BeatTracker(const BeatTracker&) = delete;
  BeatTracker& operator=(const BeatTracker&) = delete;
  /** Consume finite mono PCM; no future samples are accessed.
   * @param samples Borrowed mono PCM in frame order.
   * @param epoch Source epoch, changed when source timing resets.
   * @param first Frame position of the first sample within the epoch.
   * @param observer Optional callback for each detected event; not retained. Events without an observer are discarded.
   * @param discontinuity Force reacquisition even for contiguous PCM.
   * @throws std::exception on invalid PCM, overflow, initialization or observer
   * failure. Discard the tracker after any exception. The
   * aubio counter is 32-bit; after its maximum safe span is exhausted,
   * create a new tracker with a new epoch rather than wrapping time.
   */
  void process(std::span<const float> samples, std::uint64_t epoch, std::uint64_t first,
               const Observer& observer = {}, bool discontinuity = false);
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
