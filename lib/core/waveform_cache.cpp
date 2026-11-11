#include "beatglass/core/waveform_cache.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace beatglass {

std::vector<WaveformBucket> extract_waveform_envelope(std::span<const float> samples,
                                                      std::uint32_t bucket_count) {
  if (samples.empty() || bucket_count == 0 || bucket_count > samples.size())
    throw std::invalid_argument("waveform extraction requires nonempty PCM and valid zoom");
  if (std::any_of(samples.begin(), samples.end(), [](float value) {
        return !std::isfinite(value) || value < -1.0f || value > 1.0f;
      }))
    throw std::invalid_argument("waveform PCM must be finite and normalized");
  std::vector<WaveformBucket> result;
  result.reserve(bucket_count);
  for (std::uint32_t bucket = 0; bucket < bucket_count; ++bucket) {
    const auto begin = static_cast<std::size_t>(bucket) * samples.size() / bucket_count;
    const auto end = static_cast<std::size_t>(bucket + 1) * samples.size() / bucket_count;
    auto range = std::minmax_element(samples.begin() + static_cast<std::ptrdiff_t>(begin),
                                     samples.begin() + static_cast<std::ptrdiff_t>(end));
    result.push_back({*range.first, *range.second});
  }
  return result;
}

struct WaveformCache::Entry {
  std::string identity;
  std::uint64_t revision{};
  std::uint32_t zoom{};
  std::uint64_t last_used{};
  std::vector<WaveformBucket> envelope;
};

WaveformCache::WaveformCache(std::size_t max_entries) : max_entries_(max_entries) {
  if (max_entries_ == 0) throw std::invalid_argument("waveform cache capacity must be nonzero");
}

const std::vector<WaveformBucket>& WaveformCache::get(std::string identity,
                                                       std::uint64_t revision,
                                                       std::uint32_t zoom,
                                                       std::span<const float> samples) {
  if (identity.empty() || revision == 0 || zoom == 0)
    throw std::invalid_argument("waveform cache identity is incomplete");
  auto found = std::find_if(entries_.begin(), entries_.end(), [&](const Entry& entry) {
    return entry.identity == identity && entry.revision == revision && entry.zoom == zoom;
  });
  if (found != entries_.end()) {
    found->last_used = ++use_clock_;
    return found->envelope;
  }
  Entry entry{std::move(identity), revision, zoom, ++use_clock_, extract_waveform_envelope(samples, zoom)};
  if (entries_.size() == max_entries_) {
    auto victim = std::min_element(entries_.begin(), entries_.end(), [](const Entry& left, const Entry& right) {
      return left.last_used < right.last_used;
    });
    *victim = std::move(entry);
    return victim->envelope;
  } else {
    entries_.push_back(std::move(entry));
  }
  return entries_.back().envelope;
}

void WaveformCache::invalidate(const std::string& identity) {
  entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [&](const Entry& entry) {
    return entry.identity == identity;
  }), entries_.end());
}

void WaveformCache::clear() noexcept { entries_.clear(); }
std::size_t WaveformCache::size() const noexcept { return entries_.size(); }

}
