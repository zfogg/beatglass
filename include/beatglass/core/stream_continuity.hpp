#pragma once
#include "beatglass/core/audio.hpp"
#include <cstdint>
#include <limits>
namespace beatglass {
enum class StreamTransition { invalid, acquire, contiguous, reacquire };
// Consumer-thread state machine. Never called from a capture callback.
// Each accepted block updates the expected next frame. Invalid input does not
// advance state. A format change inside an epoch is invalid: the source must
// create a new epoch. Once a gap is observed, callers reset all estimators.
class StreamContinuity {
public:
  StreamTransition observe(const AudioBlock& block) noexcept {
    const auto& f = block.format;
    if (!f.sample_rate || !f.channels || block.samples.empty() || block.samples.size() % f.channels) return StreamTransition::invalid;
    const auto frames = block.samples.size() / f.channels;
    if (frames > std::numeric_limits<std::uint64_t>::max() - block.first_frame) return StreamTransition::invalid;
    if (started_ && block.epoch == epoch_ && (format_.sample_rate != f.sample_rate || format_.channels != f.channels || format_.layout != f.layout)) return StreamTransition::invalid;
    const auto transition = !started_ ? StreamTransition::acquire :
      (block.discontinuity || block.epoch != epoch_ || block.first_frame != next_) ? StreamTransition::reacquire : StreamTransition::contiguous;
    started_ = true; epoch_ = block.epoch; format_ = f; next_ = block.first_frame + frames;
    return transition;
  }
  void stop() noexcept { started_ = false; }
private:
  bool started_ = false; std::uint64_t epoch_ = 0, next_ = 0; AudioFormat format_{};
};
}
