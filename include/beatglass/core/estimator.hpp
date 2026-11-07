#pragma once
#include "beatglass/core/audio.hpp"
#include "beatglass/core/results.hpp"
namespace beatglass {
// Implementations consume only observed samples and reset on epoch changes.
/// Provisional estimator interface. Implementations consume observed samples and reset on epoch changes; thread safety and error behavior are not specified.
class Estimator {
public:
  virtual ~Estimator() = default;
/// Reset for an epoch and audio format.
  virtual void reset(std::uint64_t epoch, AudioFormat format) = 0;
/// Consume a borrowed audio block.
  virtual void process(const AudioBlock& block) = 0;
/// Return a snapshot by value.
  virtual Snapshot snapshot() const = 0;
};
}
