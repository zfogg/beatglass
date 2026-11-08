#pragma once
#ifdef _WIN32
#include <string>
#include <vector>
namespace beatglass::windows {
/** Stable point-in-time description of an available waveOut playback device. */
struct WaveOutDevice { unsigned id{}; std::string name; };
/** Enumerate waveOut devices; unavailable or malformed names are omitted. */
std::vector<WaveOutDevice> enumerate_waveout_devices();
}
#endif
