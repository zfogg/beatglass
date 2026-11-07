#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
namespace beatglass {
struct FileMetadata { std::uint32_t sample_rate{}, channels{}; std::uint64_t frames{}; std::string format; };
struct TempoResult { FileMetadata metadata; std::optional<float> bpm; unsigned beats{}; };
FileMetadata read_metadata(const std::filesystem::path& path);
TempoResult analyze_file(const std::filesystem::path& path);
// Faster-than-real-time causal replay: callback receives only fully observed
// frames and the current estimate, on the caller's thread. No pacing is implied.
using ReplayObserver = std::function<void(std::uint64_t observed_frames, const TempoResult&)>;
TempoResult replay_file(const std::filesystem::path& path, const ReplayObserver& observer);
}
