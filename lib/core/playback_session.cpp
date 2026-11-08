#include "beatglass/core/playback_session.hpp"
namespace beatglass {
bool PlaybackSession::start(std::uint64_t epoch) noexcept {
  if (state_ != PlaybackState::stopped || !epoch) return false;
  state_ = PlaybackState::playing; epoch_ = epoch; next_frame_ = 0; return true;
}
bool PlaybackSession::pause() noexcept { if (state_ != PlaybackState::playing) return false; state_ = PlaybackState::paused; return true; }
void PlaybackSession::stop() noexcept { state_ = PlaybackState::stopped; epoch_ = next_frame_ = 0; }
bool PlaybackSession::seek(std::uint64_t frame, std::uint64_t new_epoch) noexcept {
  if (state_ == PlaybackState::stopped || !new_epoch || new_epoch <= epoch_) return false;
  epoch_ = new_epoch; next_frame_ = frame; state_ = PlaybackState::playing; return true;
}
bool PlaybackSession::accept(std::uint64_t epoch, std::uint64_t first_frame) noexcept {
  if (state_ != PlaybackState::playing || epoch != epoch_ || first_frame != next_frame_) return false;
  ++next_frame_; return true;
}
}
