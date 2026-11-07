#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>

namespace beatglass::coreaudio {
/** Provisional, non-installed Core Audio transfer format. One block owns its PCM. Host ticks are in the macOS host clock domain and are valid only when host_time_valid is true; they are not nanoseconds. first_frame counts frames since this capture epoch began. */
struct CaptureBlock {
  static constexpr std::size_t max_samples = 8192;
  std::uint64_t epoch{}, first_frame{}, host_time{};
  std::uint32_t sample_rate{}, channels{}, sample_count{};
  bool host_time_valid{}, discontinuity{};
  std::array<float, max_samples> samples{};
};

/** Private bounded single-producer/single-consumer PCM handoff. The callback is the only producer, the analysis worker is the only consumer, and neither may overlap reset/destruction. The producer copies only and performs no allocation or locks. Rejected whole blocks advance the frame timeline and cause the next accepted block to report discontinuity. */
class CaptureFifo {
public:
  /** Set immutable epoch/format before callbacks start. A new instance is required for each epoch. */
  CaptureFifo(std::uint64_t epoch, std::uint32_t sample_rate, std::uint32_t channels) noexcept;
  /** Copy complete interleaved float frames. Returns false on an oversized block, malformed input, full queue or timeline exhaustion; no memory is allocated. */
  bool push(std::span<const float> samples, std::uint64_t host_time, bool host_time_valid, bool discontinuity = false) noexcept;
  /** Copy the next block into consumer-owned storage, or return false when empty. */
  bool pop(CaptureBlock& block) noexcept;
  /** Number of complete frames rejected by the producer (saturates at uint64 maximum). */
  std::uint64_t dropped_frames() const noexcept;
  /** Return true after frame-position exhaustion; the stream must stop rather than wrap. */
  bool exhausted() const noexcept;
private:
  static constexpr std::size_t slots = 16;
  std::array<CaptureBlock, slots> blocks_{};
  std::atomic<std::size_t> read_{0}, write_{0};
  std::atomic<std::uint64_t> dropped_{0};
  std::uint64_t epoch_, next_frame_{};
  std::uint32_t sample_rate_, channels_;
  bool discontinuity_{};
  std::atomic<bool> exhausted_{false};
};
}
