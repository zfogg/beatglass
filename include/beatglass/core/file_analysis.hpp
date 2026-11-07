#pragma once
#include <cstdint>
#include <filesystem>
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
}
