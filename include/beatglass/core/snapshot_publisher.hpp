#pragma once
#include "beatglass/core/presentation.hpp"
#include "beatglass/core/tracking_machine.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
namespace beatglass {
/**
 * Worker-owned latest-snapshot publisher with bounded item counts.
 *
 * One producer calls begin_acquisition/publish; any number of readers may call
 * latest. Methods may allocate and the shared-pointer atomic may lock: this
 * class must not be used by the real-time capture callback. Readers own an
 * immutable shared copy; replacing the latest snapshot does not invalidate
 * copies already held by a reader. Producers provide ordered event histories,
 * including any prior events they intend to retain.
 */
class SnapshotPublisher {
public:
  /** Set maximum retained beats/chords and tempo/key candidates per list.
   * Text fields are limited to 128 bytes. A zero limit is permitted.
   */
  explicit SnapshotPublisher(std::size_t history_limit, std::size_t candidate_limit)
      : history_limit_(history_limit), candidate_limit_(candidate_limit) {}
  /** Begin an acquisition and clear the latest publication.
   * @param token Current token from the worker-owned tracking machine. Each
   *              new acquisition must use its new generation.
   */
  void begin_acquisition(AcquisitionToken token) noexcept {
    epoch_ = token.epoch; generation_ = token.generation; active_ = true; have_frame_ = false;
    std::atomic_store_explicit(&latest_, PublishedSnapshot{}, std::memory_order_release);
  }
  /** Invalidate pending publications and clear latest on stop, silence or error. */
  void clear() noexcept {
    active_ = false; have_frame_ = false;
    std::atomic_store_explicit(&latest_, PublishedSnapshot{}, std::memory_order_release);
  }
  /** Publish an observed, ordered snapshot on the producer thread.
   * @param token Acquisition token captured when this result was produced.
   * @param snapshot A value owned by the caller; moved into immutable storage.
   * @return False for inactive, mismatched or stale tokens/epochs/frames, events
   *         outside the exclusive observed boundary, empty chord intervals,
   *         unordered events or text over 128 bytes. Beats must be strictly
   *         before the boundary; nonempty half-open chords may end at it.
   *         Invalid snapshots leave the previous publication unchanged.
   * @throws std::bad_alloc if allocation for the bounded copy fails.
   */
  bool publish(AcquisitionToken token, Snapshot snapshot);
  /** Obtain an immutable latest snapshot on any reader thread.
   * @return Empty before publication, after clear, or immediately after begin_acquisition.
   */
  PublishedSnapshot latest() const noexcept { return std::atomic_load_explicit(&latest_, std::memory_order_acquire); }
private:
  std::size_t history_limit_, candidate_limit_;
  std::uint64_t epoch_{}, generation_{}, last_frame_{};
  bool active_{}, have_frame_{};
  PublishedSnapshot latest_{};
};
}
