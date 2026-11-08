#pragma once
#include <cstdint>
namespace beatglass {
/** Control-thread state for one FileReader playback lifetime. */
enum class PlaybackState { stopped, playing, paused };
/**
 * Serial playback handoff contract. A nonzero epoch identifies the current
 * source/seek lifetime; seek retires the old epoch and resets the frame cursor.
 * Audio workers must reject blocks whose epoch does not match token().
 */
class PlaybackSession {
public:
  /** Start a stopped session with a coordinator-issued epoch. */
  bool start(std::uint64_t epoch) noexcept;
  /** Pause without retiring the source epoch. */
  bool pause() noexcept;
  /** Resume a paused session without retiring its source epoch. */
  bool resume() noexcept;
  /** Stop and retire the current source. */
  void stop() noexcept;
  /** Seek to a frame using a strictly newer nonzero epoch. */
  bool seek(std::uint64_t frame, std::uint64_t new_epoch) noexcept;
  /** Replace the active source at frame zero with a strictly newer epoch. */
  bool replace(std::uint64_t new_epoch) noexcept;
  /** Accept a FileReader block only from the active epoch and in order. */
  bool accept(std::uint64_t epoch, std::uint64_t first_frame) noexcept;
  /** Accept an ordered block and advance by its actual frame count. */
  bool accept(std::uint64_t epoch, std::uint64_t first_frame,
              std::uint64_t frame_count) noexcept;
  PlaybackState state() const noexcept { return state_; }
  std::uint64_t epoch() const noexcept { return epoch_; }
  std::uint64_t next_frame() const noexcept { return next_frame_; }
private:
  PlaybackState state_{PlaybackState::stopped};
  std::uint64_t epoch_{}, next_frame_{};
};
}
