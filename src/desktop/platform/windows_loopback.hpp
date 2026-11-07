#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

namespace beatglass::windows {
/** One owned interleaved float packet copied from a selected render endpoint. Positions count delivered or dropped complete frames from the capture epoch's origin; WASAPI device-position jumps are not included in that count. A discontinuity or invalid timestamp requires the consumer to discard tracking evidence and reacquire. */
struct LoopbackPacket {
  /** Maximum interleaved sample values in one packet. */
  static constexpr unsigned max_samples = 8192;
  /** Owned interleaved float PCM; only the first sample_count values are valid. */
  std::array<float, max_samples> samples{};
  std::uint32_t sample_count{}; /**< Number of interleaved sample values, not frames. */
  std::uint32_t sample_rate{}; /**< Frames per second. */
  std::uint32_t channels{}; /**< Interleaved values per frame. */
  std::uint64_t epoch{}; /**< Caller-allocated stream epoch, never reused while evidence can remain in flight. */
  std::uint64_t first_frame{}; /**< Epoch-relative count of frames processed by the backend, including queue drops but excluding unknown device gaps. */
  std::uint64_t qpc_100ns{}; /**< Windows QPC timestamp for the first frame, in 100-nanosecond units, meaningful only when timestamp_valid is true. */
  bool timestamp_valid{}; /**< False when WASAPI flags a timestamp error; qpc_100ns must then be ignored. */
  bool discontinuity{}; /**< True on a WASAPI discontinuity, timestamp error or preceding queue drop; downstream must reset and reacquire. */
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
  /** Stop and join the capture thread. Call on the control thread; queued packets are discarded. */
  virtual void stop();
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
