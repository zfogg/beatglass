#pragma once
#include "beatglass/core/audio.hpp"
#include "beatglass/core/results.hpp"
#include <vector>
namespace beatglass {
// Worker-owned. process() consumes observed PCM only; spans expire on return.
// A reset discards all evidence before a new epoch, format, seek or gap.
/** Provisional worker-owned strategy. Reset discards evidence on a new epoch, format, seek, or gap; process consumes borrowed observed PCM. */
class TempoEstimator {
public:
  virtual ~TempoEstimator() = default;
  /** Discard prior evidence and prepare for the given epoch and format. */
  virtual void reset(std::uint64_t epoch, AudioFormat format) = 0;
  /** Consume observed PCM; the borrowed block and samples expire on return. */
  virtual void process(const AudioBlock&) = 0;
  /** Return current candidates by value; scores are uncalibrated strengths. */
  virtual std::vector<Candidate> candidates() const = 0;
  /** Take the beat events made available by the implementation. Buffering and empty-result semantics remain unspecified. */
  virtual std::vector<Beat> take_beats() = 0;
};
/** Provisional worker-owned key strategy with the same reset and borrowed PCM rules. */
class KeyEstimator {
public:
  virtual ~KeyEstimator() = default;
  /** Discard prior evidence and prepare for the given epoch and format. */
  virtual void reset(std::uint64_t epoch, AudioFormat format) = 0;
  /** Consume observed PCM; the borrowed block and samples expire on return. */
  virtual void process(const AudioBlock&) = 0;
  /** Return current candidates by value; scores are uncalibrated strengths. */
  virtual std::vector<Candidate> candidates() const = 0;
};
/** Provisional worker-owned chord strategy with the same reset and borrowed PCM rules. */
class ChordEstimator {
public:
  virtual ~ChordEstimator() = default;
  /** Discard prior evidence and prepare for the given epoch and format. */
  virtual void reset(std::uint64_t epoch, AudioFormat format) = 0;
  /** Consume observed PCM; the borrowed block and samples expire on return. */
  virtual void process(const AudioBlock&) = 0;
  /** Take chord segments made available by the implementation. Buffering and empty-result semantics remain unspecified. */
  virtual std::vector<ChordSegment> take_segments() = 0;
};
}
