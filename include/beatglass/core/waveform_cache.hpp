#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace beatglass {

/** One min/max envelope bucket extracted from source PCM. */
struct WaveformBucket {
  float minimum{};
  float maximum{};
};

/** Extract bounded extrema from finite normalized source PCM.
 * The input is borrowed for the call and is never retained. `bucket_count`
 * must be nonzero; adjacent buckets cover the full input, including a short
 * final bucket. Throws std::invalid_argument for invalid input.
 */
std::vector<WaveformBucket> extract_waveform_envelope(std::span<const float> samples,
                                                      std::uint32_t bucket_count);

/** Cache of source-derived envelopes at multiple zoom levels.
 * Own on a non-audio worker thread. Keys include the caller's stable source
 * identity and revision, so replacing or rescanning a source must use a new
 * revision (or call invalidate). The cache owns returned vectors and is
 * bounded by `max_entries`; eviction is least-recently-used by request.
 * This class performs no I/O and is not thread-safe.
 */
class WaveformCache {
public:
  explicit WaveformCache(std::size_t max_entries = 32);
  /** Return a cached envelope or extract and cache it from borrowed PCM. */
  const std::vector<WaveformBucket>& get(std::string source_identity,
                                         std::uint64_t source_revision,
                                         std::uint32_t zoom,
                                         std::span<const float> samples);
  /** Remove every zoom level for one source identity. */
  void invalidate(const std::string& source_identity);
  /** Remove all cached source envelopes. */
  void clear() noexcept;
  std::size_t size() const noexcept;

private:
  struct Entry;
  std::size_t max_entries_;
  std::uint64_t use_clock_{};
  std::vector<Entry> entries_;
};

}
