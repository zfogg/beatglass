#include "beatglass/core/playback_session.hpp"
#include <limits>
namespace beatglass {
bool PlaybackSession::start(std::uint64_t epoch) noexcept {
  if (state_ != PlaybackState::stopped || !epoch) return false;
  state_ = PlaybackState::playing; epoch_ = epoch; next_frame_ = 0; return true;
}
bool PlaybackSession::pause() noexcept { if (state_ != PlaybackState::playing) return false; state_ = PlaybackState::paused; return true; }
bool PlaybackSession::resume() noexcept { if (state_ != PlaybackState::paused) return false; state_ = PlaybackState::playing; return true; }
void PlaybackSession::stop() noexcept { state_ = PlaybackState::stopped; epoch_ = next_frame_ = 0; }
bool PlaybackSession::seek(std::uint64_t frame, std::uint64_t new_epoch) noexcept {
  if (state_ == PlaybackState::stopped || !new_epoch || new_epoch <= epoch_) return false;
  epoch_ = new_epoch; next_frame_ = frame; state_ = PlaybackState::playing; return true;
}
bool PlaybackSession::replace(std::uint64_t new_epoch) noexcept {
  if (state_ == PlaybackState::stopped || !new_epoch || new_epoch <= epoch_) return false;
  epoch_ = new_epoch; next_frame_ = 0; state_ = PlaybackState::playing; return true;
}
bool PlaybackSession::accept(std::uint64_t epoch, std::uint64_t first_frame) noexcept {
  return accept(epoch, first_frame, 1);
}
bool PlaybackSession::accept(std::uint64_t epoch, std::uint64_t first_frame,
                             std::uint64_t frame_count) noexcept {
  if (state_ != PlaybackState::playing || epoch != epoch_ || first_frame != next_frame_ || !frame_count) return false;
  if (frame_count > std::numeric_limits<std::uint64_t>::max() - next_frame_) return false;
  next_frame_ += frame_count; return true;
}
}
