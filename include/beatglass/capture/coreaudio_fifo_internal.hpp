#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>

namespace beatglass::coreaudio {
/** Result of one nonblocking callback gate entry attempt. */
enum class CallbackEntry { entered, closed, busy };
/** Lock-free gate between the single native callback and its control owner. The callback attempts entry once and never waits; the owner closes the gate, then waits off the audio thread until drained before releasing callback-owned buffers. Closing does not unregister the native callback or guarantee that the operating system stopped invoking it. The gate itself must remain alive until the native callback is destroyed. */
class CallbackGate {
public:
  /** Attempt entry once. Busy means concurrent entry or a race with closure; the caller must report a fault if still active. */
  CallbackEntry try_enter() noexcept;
  /** Leave exactly once after a successful entry. */
  void leave() noexcept;
  /** Prevent future entries; safe to call repeatedly on the control owner. */
  void close() noexcept;
  /** True once closed and all successful entries have left. */
  bool drained() const noexcept;
  /** Reopen only after the old native callback has been destroyed and no callback can reference the gate. */
  void reopen() noexcept;
private:
  std::atomic<std::uint32_t> state_{0};
};

/** Internal control-thread teardown operations. Each must be idempotent, nonthrowing, and return false if its resource remains live. */
struct CleanupOps {
  /** Stop native I/O. */
  bool (*stop_io)(void*) noexcept;
  /** Destroy the IOProc. */
  bool (*destroy_io)(void*) noexcept;
  /** Destroy the aggregate device. */
  bool (*destroy_aggregate)(void*) noexcept;
  /** Destroy the process tap. */
  bool (*destroy_tap)(void*) noexcept;
};
/** Run teardown in dependency order, stopping at the first failure. Safe retry depends on each operation retaining its resource state until it succeeds. The callback gate must already be closed and drained. */
bool cleanup_in_order(void* context, const CleanupOps& operations) noexcept;

/** Provisional, non-installed Core Audio transfer format. One block owns its PCM. Host ticks are in the macOS host clock domain and are valid only when host_time_valid is true; they are not nanoseconds. source_sample_time is the native input timestamp and is meaningful only when source_sample_time_valid is true. first_frame counts frames since this capture epoch began, including whole frames rejected by the FIFO. */
struct CaptureBlock {
  static constexpr std::size_t max_samples = 8192;
  std::uint64_t epoch{}, first_frame{}, host_time{};
  double source_sample_time{};
  std::uint32_t sample_rate{}, channels{}, sample_count{};
  bool host_time_valid{}, source_sample_time_valid{}, discontinuity{};
  std::array<float, max_samples> samples{};
};

/** Producer-owned native sample-time continuity detector. It tracks only the input time supplied by Core Audio, not wall-clock time; no allocation or locking occurs. A timestamp jump, loss of previously valid time, or its return after missing time marks a discontinuity. */
class NativeTimeline {
public:
  /** Observe one callback's first-frame timestamp and complete-frame count. Non-finite timestamps are treated as invalid. Returns whether this callback begins a discontinuity. */
  bool observe(double sample_time, bool valid, std::uint32_t frames) noexcept;
private:
  double expected_{};
  bool seen_{}, last_valid_{};
};

/** Private bounded single-producer/single-consumer PCM handoff. The callback is the only producer, the analysis worker is the only consumer, and neither may overlap reset/destruction. The producer copies only and performs no allocation or locks. Rejected whole blocks advance the frame timeline and cause the next accepted block to report discontinuity. */
class CaptureFifo {
public:
  /** Set immutable epoch/format before callbacks start. A new instance is required for each epoch. */
  CaptureFifo(std::uint64_t epoch, std::uint32_t sample_rate, std::uint32_t channels) noexcept;
  /** Copy complete interleaved float frames. Returns false on an oversized block, malformed input, full queue or timeline exhaustion; no memory is allocated. */
  bool push(std::span<const float> samples, std::uint64_t host_time, bool host_time_valid, bool discontinuity = false,
            double source_sample_time = 0, bool source_sample_time_valid = false) noexcept;
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
