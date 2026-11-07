#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
namespace beatglass {
/// Decoded file properties; format is WAV or the generic audio label.
struct FileMetadata { std::uint32_t sample_rate{}, channels{}; std::uint64_t frames{}; std::string format; };
/// File metadata, optional BPM, and detected beat count.
struct TempoResult { FileMetadata metadata; std::optional<float> bpm; unsigned beats{}; };
/// Open a file and read metadata. Throws std::runtime_error on open or invalid format; other exceptions may propagate.
FileMetadata read_metadata(const std::filesystem::path& path);
/// Decode a file and run baseline tempo analysis. Each block uses its loudest channel; switches can introduce discontinuities. Throws std::runtime_error on file or decoder failure; tracker and allocation exceptions may propagate.
TempoResult analyze_file(const std::filesystem::path& path);
// Faster-than-real-time causal replay: callback receives only fully observed
// frames and the current estimate, on the caller's thread. No pacing is implied.
/// Replay observer called on the caller thread with observed frame count and current estimate. Its result reference is borrowed for the call.
using ReplayObserver = std::function<void(std::uint64_t observed_frames, const TempoResult&)>;
/// Decode and analyze faster than real time, calling the observer after observed blocks without pacing. Decoder, tracker, and observer exceptions can propagate.
TempoResult replay_file(const std::filesystem::path& path, const ReplayObserver& observer);
}
