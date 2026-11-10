#pragma once

#include <filesystem>
#include <cstdint>
#include <string>
#include <stop_token>
#include <vector>

namespace beatglass::library {

/** One visible entry in a local library folder.
 * The path is absolute when the input folder is absolute, and the entry does
 * not own or modify the referenced file. Directories are included so callers
 * can navigate; regular files are included only for the qualified WAV, FLAC,
 * MP3, and AIFF/AIF v1 formats (case-insensitive extensions).
 */
struct CatalogEntry {
  std::filesystem::path path;
  std::string name;
  /** Display title fallback derived from the filename stem; not embedded metadata. */
  std::string title;
  /** Embedded artist metadata, when available; empty during filesystem-only browse. */
  std::string artist;
  bool directory{};
  /** Byte size for regular files; zero for directories or stat failures. */
  std::uintmax_t bytes{};
  /** Supported audio container label derived from the filename extension. */
  std::string format;
  /** Duration in seconds when decoded metadata is available; -1 means unknown. */
  double duration_seconds{-1.0};
};

/** Machine-readable outcome of a synchronous catalog query. */
enum class CatalogStatus {
  ok,
  invalid_root,
  filesystem_error,
  entry_limit_exceeded,
};

/** Result of a synchronous folder query performed by a non-audio worker.
 * On filesystem failure, entries is empty and diagnostic contains the
 * platform error. The query never follows directory entries recursively.
 */
struct CatalogResult {
  std::vector<CatalogEntry> entries;
  std::string diagnostic;
  CatalogStatus status{CatalogStatus::ok};
  /** True when the caller cancelled before a complete snapshot was built. */
  bool cancelled{};
};

/** Return the canonical display format for a supported audio path.
 * The result is one of `WAV`, `FLAC`, `MP3`, or `AIFF`; an empty string means
 * the extension is not part of the qualified catalog set. This is a pure
 * filename policy helper and does not access the filesystem.
 */
std::string audio_format_for_path(const std::filesystem::path& path);

/** Enumerate one local folder for the Library view.
 * The search is case-insensitive against entry names. Results are sorted by
 * case-insensitive name, then exact name, then path for deterministic UI and
 * test behavior. This function performs filesystem I/O and must not run in an
 * audio callback; it does not create, delete, or modify user files.
 */
CatalogResult browse(const std::filesystem::path& folder,
                     std::string search = {},
                     std::size_t max_entries = 10000,
                     std::stop_token stop = {});

/** Enumerate a local folder and all descendant folders without following links.
 * The result uses the same supported-format and deterministic ordering rules
 * as browse(); filesystem errors return an empty result with a diagnostic.
 */
CatalogResult browse_recursive(const std::filesystem::path& folder,
                               std::string search = {},
                               std::size_t max_entries = 10000,
                               std::stop_token stop = {});

} // namespace beatglass::library
