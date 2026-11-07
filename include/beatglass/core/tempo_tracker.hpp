#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
namespace beatglass {
struct TempoEstimate { std::optional<float> bpm; std::uint64_t beats{}; };
// Worker-thread only. Input is mono float PCM at the configured rate. Frame
// offsets are authoritative within an epoch. Gaps, epoch changes and explicit
// discontinuities discard all partial windows and restart acquisition.
class TempoTracker {
public:
  explicit TempoTracker(std::uint32_t sample_rate);
  ~TempoTracker();
  TempoTracker(const TempoTracker&) = delete;
  TempoTracker& operator=(const TempoTracker&) = delete;
  void process(std::span<const float> samples, std::uint64_t epoch, std::uint64_t first_frame, bool discontinuity = false);
  TempoEstimate estimate() const;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
