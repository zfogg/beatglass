#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
namespace beatglass {
struct FileMetadata { std::uint32_t sample_rate{}, channels{}; std::uint64_t frames{}; std::string format; };
struct TempoResult { FileMetadata metadata; std::optional<float> bpm; unsigned beats{}; };
FileMetadata read_metadata(const std::filesystem::path& path);
TempoResult analyze_file(const std::filesystem::path& path);
}
