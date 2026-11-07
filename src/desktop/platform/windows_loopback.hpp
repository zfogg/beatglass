#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <limits>
#include <string>

namespace beatglass::windows {
/** Shared capture-loss token. The capture worker is the sole writer; GUI and analysis threads may read concurrently. Invalidation is bounded, allocation-free and lock-free. Exhaustion permanently rejects all stamps rather than wrapping. A token is not an epoch or an ordered publication revision. */
class LoopbackLossToken {
public:
  /** Snapshot on any thread. A value at exhaustion is never valid. */
  std::uint64_t stamp() const noexcept { return value_.load(std::memory_order_acquire); }
  /** True if this stamp still identifies the current, non-exhausted capture interval. */
  bool matches(std::uint64_t stamp) const noexcept { return stamp != std::numeric_limits<std::uint64_t>::max() && value_.load(std::memory_order_acquire) == stamp; }
  /** Invalidate earlier stamps on the sole capture worker; no allocation or blocking. */
  void invalidate() noexcept { const auto old=value_.load(std::memory_order_relaxed); if (old != std::numeric_limits<std::uint64_t>::max()) value_.store(old+1, std::memory_order_release); }
private:
  static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
  std::atomic<std::uint64_t> value_{0};
};

/** One owned interleaved float packet copied from a selected render endpoint. Positions count delivered or dropped complete frames from the capture epoch's origin; WASAPI device-position jumps are marked discontinuous but are not included in that count. A discontinuity or invalid timestamp requires the consumer to discard tracking evidence and reacquire. */
struct LoopbackPacket {
  /** Maximum interleaved sample values in one packet. */
  static constexpr unsigned max_samples = 8192;
  /** Owned interleaved float PCM; only the first sample_count values are valid. */
  std::array<float, max_samples> samples{};
  std::uint32_t sample_count{}; /**< Number of interleaved sample values, not frames. */
  std::uint32_t sample_rate{}; /**< Frames per second. */
  std::uint32_t channels{}; /**< Interleaved values per frame. */
  std::uint64_t epoch{}; /**< Caller-allocated stream epoch, never reused while evidence can remain in flight. */
  std::uint64_t loss_stamp{}; /**< Capture-loss token sampled for this packet; retain this stamp with derived evidence, never substitute a later snapshot. */
  std::uint64_t first_frame{}; /**< Epoch-relative count of frames processed by the backend, including queue drops but excluding unknown device gaps. */
  std::uint64_t qpc_100ns{}; /**< Windows QPC timestamp for the first frame, in 100-nanosecond units, meaningful only when timestamp_valid is true. */
  bool timestamp_valid{}; /**< False when WASAPI flags a timestamp error; qpc_100ns must then be ignored. */
  bool discontinuity{}; /**< True on a WASAPI discontinuity, device-position jump, timestamp error, position overflow or preceding queue drop; downstream must reset and reacquire. */
};

/** Shared one-attempt start authorization. Any thread may cancel; one control worker may claim. Cancellation before the claim prevents startup; after the claim it requests shutdown. Polling is asynchronous and may race cancellation; the consumer must synchronize its own generation or token gate before publishing. No operation allocates or blocks. */
class LoopbackStartAuthorization {
public:
  /** Atomically commit the sole start unless cancellation won first. */
  bool claim() noexcept { unsigned expected = 0; return state_.compare_exchange_strong(expected, 1, std::memory_order_acq_rel); }
  /** Revoke the attempt. Safe concurrently with claim and native startup. */
  void cancel() noexcept { state_.store(2, std::memory_order_release); }
  /** True after revocation. */
  bool cancelled() const noexcept { return state_.load(std::memory_order_acquire) == 2; }
private:
  std::atomic<unsigned> state_{0};
};

/** Provisional selected-endpoint capture, independent of the shared source contract. Virtual control methods allow deterministic adapter tests; production capture callbacks do not dispatch through this interface. */
class Loopback {
public:
  /** State is published atomically by the capture thread. Interrupted means the stream has stopped and buffered packets must be discarded. */
  enum class State { stopped, starting, running, interrupted };
  /** One capture-thread transition, numbered consecutively within one start attempt. An interrupted event invalidates all evidence for the epoch. */
  struct Event {
    State state{}; /**< Running or interrupted transition. */
    std::uint64_t sequence{}; /**< Consecutive number starting at one for each start attempt. */
    std::uint64_t epoch{}; /**< Caller-provided epoch for that attempt. */
    std::int32_t error_code{}; /**< HRESULT on interruption, zero when running. */
  };
  /** Construct a stopped source; the caller owns it. */
  Loopback();
  /** Stop and join before releasing buffers. Never destroy on the capture thread. */
  virtual ~Loopback();
  Loopback(const Loopback&) = delete;
  Loopback& operator=(const Loopback&) = delete;
  /** Start the opaque UTF-8 render endpoint ID using a caller-allocated, never-reused epoch. Call on the control thread while stopped; errors are observable through state and error_code. */
  virtual bool start(const std::string& endpoint_id, std::uint64_t epoch);
  /** Start with a claimed authorization retained by the capture worker. Cancellation is checked immediately before and after the audio start call, and during capture polling; earlier initialization calls are not individually checked. An already executing OS call is not preempted. */
  virtual bool start(const std::string& endpoint_id, std::uint64_t epoch, std::shared_ptr<LoopbackStartAuthorization> authorization);
  /** Stop and join the capture thread. Call on the control thread; queued packets are discarded. */
  virtual void stop();
  /** Share the loss token with consumers; retained references outlive the source. Source/session generation and revision gates are still required. GUI updates can only clear when the GUI thread runs. */
  std::shared_ptr<const LoopbackLossToken> lossToken() const noexcept;
  /** Read current state from any non-capture thread. */
  virtual State state() const;
  /** Native HRESULT from a failed stream; meaningful only after interruption. */
  std::int32_t error_code() const;
  /** Read the next ordered running or interrupted transition on the sole control consumer. Start and stop must not run concurrently with this call. The owner must apply running before consuming PCM and clear evidence on interruption. PCM and event queues have no shared ordering; retained or in-flight packets require external invalidation. Startup failure produces interrupted as the first event and maps to an error in the proposed shared lifecycle. Stop discards pending events. */
  virtual bool pop_event(Event& event);
  /** Copy the next packet on one consumer thread. Do not call concurrently with start or stop. */
  virtual bool pop(LoopbackPacket& packet);
  /** Count frames rejected because the queue was full. */
  virtual std::uint64_t dropped_frames() const;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
