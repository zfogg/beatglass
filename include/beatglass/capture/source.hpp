#pragma once
#include <functional>
#include <string>
#include <vector>
#include "beatglass/core/audio.hpp"
namespace beatglass {
enum class SourceState { stopped, starting, running, interrupted, error };
struct SourceStatus { SourceState state{SourceState::stopped}; std::string message; };
struct SourceInfo { std::string id, name; bool live{}, seekable{}; };
class AudioSource {
public:
  using Consumer = std::function<void(const AudioBlock&)>;
  virtual ~AudioSource() = default;
  virtual std::vector<SourceInfo> enumerate() = 0;
  // Consumer must copy if it needs samples after the call. start/stop are not
  // callback-thread operations; stop joins or quiesces all callbacks.
  virtual void start(const std::string& id, Consumer consumer) = 0;
  virtual void stop() noexcept = 0;
  // Read on the owning control thread; never queried by an audio callback.
  virtual SourceStatus status() const = 0;
};
}
