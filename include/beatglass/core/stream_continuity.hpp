#pragma once
#include "beatglass/core/audio.hpp"
#include <cstdint>
#include <limits>
namespace beatglass {
/** Result of inspecting a block against the previous accepted block. */
enum class StreamTransition {
  invalid, ///< Malformed block or format changed within an epoch.
  acquire, ///< First accepted block after construction or stop.
  contiguous, ///< Same epoch and format, with no missing frames.
  reacquire ///< Gap, explicit discontinuity, or a different epoch.
};
/**
 * Consumer-owned continuity state; not thread-safe or capture-callback work.
 *
 * Each accepted block updates the expected next frame. Invalid input leaves
 * the state unchanged. A format change within an epoch is invalid: the source
 * must create a new epoch. Reset estimators when reacquisition is requested.
 */
class StreamContinuity {
public:
  /**
   * Inspect borrowed block metadata and update the expected frame position.
   * @param block Interleaved PCM view valid for this call; it is not retained.
   * @return The transition, or invalid for malformed geometry or overflow.
   */
  StreamTransition observe(const AudioBlock& block) noexcept {
    const auto& f = block.format;
    if (!block.epoch || !f.sample_rate || !f.channels || block.samples.empty() || block.samples.size() % f.channels) return StreamTransition::invalid;
    const auto frames = block.samples.size() / f.channels;
    if (frames > std::numeric_limits<std::uint64_t>::max() - block.first_frame) return StreamTransition::invalid;
    if (started_ && block.epoch == epoch_ && (format_.sample_rate != f.sample_rate || format_.channels != f.channels || format_.layout != f.layout)) return StreamTransition::invalid;
    const auto transition = !started_ ? StreamTransition::acquire :
      (block.discontinuity || block.epoch != epoch_ || block.first_frame != next_) ? StreamTransition::reacquire : StreamTransition::contiguous;
    started_ = true; epoch_ = block.epoch; format_ = f; next_ = block.first_frame + frames;
    return transition;
  }
  /** Forget the active stream; the next accepted block starts acquisition. */
  void stop() noexcept { started_ = false; }
private:
  bool started_ = false; std::uint64_t epoch_ = 0, next_ = 0; AudioFormat format_{};
};
}
