#ifdef _WIN32
#include "windows_playback_session.hpp"
namespace beatglass::windows {
bool WindowsPlaybackSession::start(std::uint32_t rate, std::uint16_t channels, std::size_t max_samples, std::uint64_t epoch, std::string& error) { if(max_samples>4096){error="Playback block exceeds bounded queue capacity";return false;} if(!session_.start(epoch)) { error="Playback epoch is invalid or session is active"; return false; } if(!output_.open(rate,channels,max_samples,error)){session_.stop();return false;} return true; }
bool WindowsPlaybackSession::start_device(UINT device_id, std::uint32_t rate, std::uint16_t channels, std::size_t max_samples, std::uint64_t epoch, std::string& error) { if(max_samples>4096){error="Playback block exceeds bounded queue capacity";return false;} if(!session_.start(epoch)){error="Playback epoch is invalid or session is active";return false;} if(!output_.open_device(device_id,rate,channels,max_samples,error)){session_.stop();return false;} return true; }
bool WindowsPlaybackSession::pause() noexcept { return session_.pause(); }
bool WindowsPlaybackSession::resume() noexcept { return session_.resume(); }
void WindowsPlaybackSession::stop() noexcept { session_.stop(); output_.stop(); }
bool WindowsPlaybackSession::submit(const AudioBlock& block) noexcept { return session_.state()==PlaybackState::playing && queue_.push(block); }
bool WindowsPlaybackSession::render(std::span<float> storage) noexcept { return session_.state()==PlaybackState::playing && output_.render_queue(queue_,storage); }
}
#endif
