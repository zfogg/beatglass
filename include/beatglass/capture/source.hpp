#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>
#include "beatglass/core/audio.hpp"
namespace beatglass {
/** Provisional source lifecycle states. */
enum class SourceState { stopped, starting, running, interrupted, error };
/** Stable failure categories; backend diagnostics are supplemental. */
enum class SourceError { not_found, permission_denied, unsupported, busy, device_lost, cancelled, backend_error };
/** Identifies a start attempt within its control owner's lifetime, not an audio epoch. */
struct SourceAttempt { std::uint64_t generation{}; };
/** Source state and optional typed failure and backend diagnostic; read off the audio callback. */
struct SourceStatus { SourceState state{SourceState::stopped}; std::optional<SourceError> error; std::string message; };
/** Provisional source identity, display name and live/seekable capabilities. */
struct SourceInfo { std::string id, name; bool live{}, seekable{}; };
/** Enumeration result. An ok result can have no sources. */
enum class EnumerationOutcome { ok, cancelled, error };
/** Enumeration value; sources are meaningful only for ok and error for failure. */
struct EnumerationResult { EnumerationOutcome outcome{EnumerationOutcome::error}; std::vector<SourceInfo> sources; std::optional<SourceError> error; std::string diagnostic; };
/** Start request outcome; accepted only means startup was requested. */
struct SourceStartResult { std::optional<SourceAttempt> attempt; std::optional<SourceError> error; std::string diagnostic; };
/** Provisional source interface owned by one control thread. Consumer receives borrowed PCM and must copy samples needed after the call. Start, enumeration, status and stop are not callback operations; stop quiesces callbacks. */
class AudioSource {
public:
  /** Callback receiving a borrowed block; real-time callback restrictions apply. */
  using Consumer = std::function<void(const AudioBlock&)>;
  virtual ~AudioSource() = default;
  /** Enumerate sources, distinguishing an empty list, cancellation and error. */
  virtual EnumerationResult enumerate() = 0;
  /** Request start; return either an attempt or a typed failure, never both. */
  virtual SourceStartResult start(const std::string& id, Consumer consumer) = 0;
  // Documented manually in the API reference due to Hawkmoth's noexcept parsing.
  virtual void stop() noexcept = 0;
  /** Read status on the owning control thread. */
  virtual SourceStatus status() const = 0;
};
}
