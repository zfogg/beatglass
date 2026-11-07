#pragma once
#include "beatglass/capture/coreaudio_fifo_internal.hpp"
#include <cstdint>
#include <memory>
#include <string>

namespace beatglass::coreaudio {
/** Provisional internal state for a selected-output tap. Interrupted means the owner must invalidate published analysis, stop, and create a fresh epoch before restarting. */
enum class TapState { stopped, running, interrupted, error };
/** Control-thread status with a diagnostic. Error strings may contain Core Audio status codes and do not classify permissions reliably. */
struct TapStatus { TapState state{TapState::stopped}; std::string message; };
/** Private selected-output, first-stream tap. The owner serializes start, status, pop, and stop on one non-audio thread. A single Core Audio IOProc copies float PCM into a bounded SPSC FIFO; stop quiesces the IOProc before freeing it. This is not yet the shared AudioSource contract. */
class OutputTap {
public:
  /** Construct stopped. */
  OutputTap();
  /** Stop and release Core Audio resources; destruction must occur on the control owner. */
  ~OutputTap();
  OutputTap(const OutputTap&) = delete;
  OutputTap& operator=(const OutputTap&) = delete;
  /** Start the selected output UID's first stream without muting it. Caller owns unique nonzero process-session epochs. Returns false on unsupported OS/format, setup failure, or invalid parameters; status gives a diagnostic. System audio permission may be prompted by macOS and a successful start does not prove permission or non-silent data. */
  bool start(const std::string& uid, std::uint64_t epoch);
  /** Check device liveness and tap format on the control owner. A change/failure invalidates this stream and returns interrupted; stop and restart with a fresh epoch. Invoke regularly even while not popping. */
  TapStatus status();
  /** Copy the next block only while status remains running. The returned block is caller-owned; the consumer must invalidate already-delivered data on interruption. */
  bool pop(CaptureBlock& block);
  /** Stop and quiesce the callback; safe to call repeatedly. */
  void stop() noexcept;
  /** Return dropped complete-frame count for the current stream on the control owner. */
  std::uint64_t dropped_frames() const noexcept;
private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
}
