#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <optional>
namespace beatglass::library {
/** Read-only details shown for one local audio file; source ownership remains with the caller. */
struct FileDetails { std::filesystem::path path; std::string name, extension; std::uintmax_t size_bytes{}; std::int64_t modified_ns{}; };
/** Inspect one regular supported audio file. Returns nullopt for missing, non-audio, or unreadable paths. */
std::optional<FileDetails> inspect_file(const std::filesystem::path& path);
}
