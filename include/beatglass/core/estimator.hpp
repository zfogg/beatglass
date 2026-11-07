#pragma once
#include "beatglass/core/audio.hpp"
#include "beatglass/core/results.hpp"
namespace beatglass {
// Implementations consume only observed samples and reset on epoch changes.
class Estimator {
public:
  virtual ~Estimator() = default;
  virtual void reset(std::uint64_t epoch, AudioFormat format) = 0;
  virtual void process(const AudioBlock& block) = 0;
  virtual Snapshot snapshot() const = 0;
};
}
