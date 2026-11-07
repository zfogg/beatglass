#pragma once
#include "beatglass/core/audio.hpp"
#include "beatglass/core/results.hpp"
#include <vector>
namespace beatglass {
// Worker-owned. process() consumes observed PCM only; spans expire on return.
// A reset discards all evidence before a new epoch, format, seek or gap.
class TempoEstimator {
public:
  virtual ~TempoEstimator() = default;
  virtual void reset(std::uint64_t epoch, AudioFormat format) = 0;
  virtual void process(const AudioBlock&) = 0;
  virtual std::vector<Candidate> candidates() const = 0;
  virtual std::vector<Beat> take_beats() = 0;
};
class KeyEstimator {
public:
  virtual ~KeyEstimator() = default;
  virtual void reset(std::uint64_t epoch, AudioFormat format) = 0;
  virtual void process(const AudioBlock&) = 0;
  virtual std::vector<Candidate> candidates() const = 0;
};
class ChordEstimator {
public:
  virtual ~ChordEstimator() = default;
  virtual void reset(std::uint64_t epoch, AudioFormat format) = 0;
  virtual void process(const AudioBlock&) = 0;
  virtual std::vector<ChordSegment> take_segments() = 0;
};
}
