#pragma once
#include <functional>
#include <string>
#include <vector>
#include "beatglass/core/audio.hpp"
namespace beatglass {
/** Provisional source lifecycle states. */
enum class SourceState { stopped, starting, running, interrupted, error };
/** Source state and backend-provided message. */
struct SourceStatus { SourceState state{SourceState::stopped}; std::string message; };
/** Provisional source identity, display name and live/seekable capabilities. */
struct SourceInfo { std::string id, name; bool live{}, seekable{}; };
/** Provisional source interface. Consumer receives borrowed audio and must copy samples needed after the call. start and stop are not callback-thread operations; stop quiesces callbacks. Error reporting and concurrency beyond this are not yet specified. */
class AudioSource {
public:
/** Callback receiving a borrowed block. */
  using Consumer = std::function<void(const AudioBlock&)>;
  virtual ~AudioSource() = default;
/** Enumerate available sources; backend error behavior is not specified. */
  virtual std::vector<SourceInfo> enumerate() = 0;
  // Consumer must copy if it needs samples after the call. start/stop are not
  // callback-thread operations; stop joins or quiesces all callbacks.
/** Start the named source with a consumer; backend failure behavior is not specified. */
  virtual void start(const std::string& id, Consumer consumer) = 0;
// Stop the source and quiesce its callbacks.
  virtual void stop() noexcept = 0;
  // Read on the owning control thread; never queried by an audio callback.
/** Read status on the owning control thread, never from the audio callback. */
  virtual SourceStatus status() const = 0;
};
}
