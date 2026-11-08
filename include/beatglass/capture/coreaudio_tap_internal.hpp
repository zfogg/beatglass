#pragma once
#include "beatglass/capture/coreaudio_fifo_internal.hpp"
#include "beatglass/core/audio.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace beatglass::coreaudio {
/** Provisional internal state for a selected-output tap. Interrupted means the owner must invalidate published analysis, stop, and create a fresh epoch before restarting. */
enum class TapState { stopped, running, interrupted, error };
/** Provisional internal failure categories. Permission denial is used only when Core Audio returns its explicit permission error; silence and successful start do not prove permission state. */
enum class TapFailure { invalid_argument, not_found, permission_denied, unsupported, device_lost, backend_error };
/** Control-thread status with a supplemental diagnostic and optional failure category.
 *
 * A running status is only a point-in-time observation: callers must poll it
 * before accepting a block and treat any interruption, callback fault or
 * format change as invalidating the current epoch. A stopped status means
 * cleanup completed; an error status may retain native resources and requires
 * a successful stop before the adapter can be destroyed safely.
 */
struct TapStatus { TapState state{TapState::stopped}; std::string message; std::optional<TapFailure> failure; };
/** Classify an actual Core Audio OSStatus for a failed operation. Unknown values remain backend_error; this never infers permission denial from audio samples. */
TapFailure classify_osstatus(std::int32_t status) noexcept;
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
  /** Check device liveness and tap format on the control owner. A change/failure invalidates this stream and returns interrupted; stop and restart with a fresh epoch. Invoke regularly even while not popping. This is the only operation that translates callback faults and native liveness changes into an observable control-thread status; it does not block for queued PCM. */
  TapStatus status();
  /** Return the current epoch format after successful startup, otherwise empty. */
  std::optional<AudioFormat> format() const noexcept;
  /** Copy the next block only while status remains running. The returned block is caller-owned; the consumer must invalidate already-delivered data on interruption. */
  bool pop(CaptureBlock& block);
  /** Attempt to stop and quiesce the callback; safe to call repeatedly. If Core Audio refuses, status becomes error and the still-live callback context is retained rather than freed; destroying the adapter in that state intentionally leaks its native resources. */
  bool stop() noexcept;
  /** Return the saturated count of complete frames rejected by the current stream's bounded FIFO on the control owner. A nonzero count means the next accepted block is discontinuous; FIFO exhaustion also causes status() to report interruption and requires a fresh epoch. */
  std::uint64_t dropped_frames() const noexcept;
private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
}
