#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

namespace beatglass::windows {
/** One owned interleaved float packet copied from a selected render endpoint. */
struct LoopbackPacket {
  static constexpr unsigned max_samples = 8192;
  std::array<float, max_samples> samples{};
  std::uint32_t sample_count{}, sample_rate{}, channels{};
  std::uint64_t epoch{}, first_frame{}, qpc_100ns{};
  bool timestamp_valid{}, discontinuity{};
};

/** Provisional selected-endpoint capture, independent of the shared source contract. */
class Loopback {
public:
  /** State is published atomically by the capture thread. Interrupted means the stream has stopped and buffered packets must be discarded. */
  enum class State { stopped, starting, running, interrupted };
  /** Construct a stopped source; the caller owns it. */
  Loopback();
  /** Stop and join before releasing buffers. Never destroy on the capture thread. */
  ~Loopback();
  Loopback(const Loopback&) = delete;
  Loopback& operator=(const Loopback&) = delete;
  /** Start the opaque UTF-8 render endpoint ID using a caller-allocated, never-reused epoch. Call on the control thread while stopped; errors are observable through state and error_code. */
  bool start(const std::string& endpoint_id, std::uint64_t epoch);
  /** Stop and join the capture thread. Call on the control thread; queued packets are discarded. */
  void stop();
  /** Read current state from any non-capture thread. */
  State state() const;
  /** Native HRESULT from a failed stream; meaningful only after interruption. */
  std::int32_t error_code() const;
  /** Copy the next packet on one consumer thread. Do not call concurrently with start or stop. */
  bool pop(LoopbackPacket& packet);
  /** Count frames rejected because the queue was full. */
  std::uint64_t dropped_frames() const;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
