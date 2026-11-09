#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <optional>
namespace beatglass::library {
/** Read-only details shown for one local audio file; source ownership remains with the caller. */
struct FileDetails { std::filesystem::path path; std::string name, extension; std::uintmax_t size_bytes{}; std::int64_t modified_ns{}; /** True when the current file reader can decode this container for analysis. */ bool analysis_supported{}; };
/**
 * Inspect one regular supported audio file. The accepted extensions are
 * WAV, FLAC, MP3, OGG, AIFF/AIF, and M4A (case-insensitive). The returned path
 * is the caller's path spelling, the filename is the final component, and
 * the extension is normalized to lowercase. `analysis_supported` is false for
 * metadata-only formats such as OGG and M4A until their decoder path is
 * selected and qualified. Returns nullopt for missing,
 * non-regular, unsupported, or timestamp-unreadable paths. This is a
 * filesystem-only query and does not decode or modify the source.
 */
std::optional<FileDetails> inspect_file(const std::filesystem::path& path);
}
