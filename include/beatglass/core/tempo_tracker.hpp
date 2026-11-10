#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>
namespace beatglass {
/** Optional estimated BPM and count of detected beats since acquisition/reset. */
struct TempoEstimate {
  std::optional<float> bpm;
  std::uint64_t beats{};
  /** Recent detected beat boundaries, oldest first, bounded by the tracker. */
  std::vector<std::uint64_t> beat_frames;
};
// Worker-thread only. Input is mono float PCM at the configured rate. Frame
// offsets are authoritative within an epoch. Gaps, epoch changes and explicit
// discontinuities discard all partial windows and restart acquisition.
/** Worker-thread-only aubio baseline for mono float PCM at the configured rate. Gaps, epoch changes, and explicit discontinuities discard partial windows and restart acquisition. No thread synchronization is provided. */
class TempoTracker {
public:
/** Create a tracker; zero rate throws std::invalid_argument and aubio initialization failure throws std::runtime_error. */
  explicit TempoTracker(std::uint32_t sample_rate);
  ~TempoTracker();
  TempoTracker(const TempoTracker&) = delete;
  TempoTracker& operator=(const TempoTracker&) = delete;
/** Process samples at the given epoch and first frame. Frame position overflow throws std::overflow_error; reset failure throws std::runtime_error. */
  void process(std::span<const float> samples, std::uint64_t epoch, std::uint64_t first_frame, bool discontinuity = false);
/** Return the current estimate by value; BPM can be absent while acquiring. */
  TempoEstimate estimate() const;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
