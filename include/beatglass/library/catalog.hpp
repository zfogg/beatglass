#pragma once

#include <filesystem>
#include <cstdint>
#include <string>
#include <stop_token>
#include <vector>
#include <functional>

namespace beatglass::library {

/** One visible entry in a local library folder.
 * The path is absolute when the input folder is absolute, and the entry does
 * not own or modify the referenced file. Directories are included so callers
 * can navigate; regular files are included only for the qualified WAV, FLAC,
 * MP3, and AIFF/AIF v1 formats (case-insensitive extensions).
 */
struct CatalogEntry {
  std::filesystem::path path;
  /** Stable path relative to the query root; never contains machine roots. */
  std::filesystem::path relative_path;
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
  /** Normalized absolute root used to produce this snapshot. */
  std::filesystem::path query_root;
  std::vector<CatalogEntry> entries;
  std::string diagnostic;
  CatalogStatus status{CatalogStatus::ok};
  /** True when the caller cancelled before a complete snapshot was built. */
  bool cancelled{};
};

/** Progress reported by a multi-root scan. Values are snapshots owned by the
 * caller; the callback runs on the scanning worker and must not block. */
struct CatalogScanProgress {
  std::size_t roots_completed{};
  std::size_t roots_total{};
  std::size_t entries_seen{};
};

/** Outcome for one configured root. A missing root is reported here while
 * other roots continue; no entries from a failed root are published. */
struct CatalogRootResult {
  std::filesystem::path root;
  CatalogStatus status{CatalogStatus::ok};
  std::string diagnostic;
  std::size_t entries_seen{};
};

/** Combined result of scanning configured roots recursively. */
struct CatalogScanResult {
  std::vector<CatalogEntry> entries;
  std::vector<CatalogRootResult> roots;
  std::string diagnostic;
  bool cancelled{};
};

/** Scan options for scan_roots(). The work queue is bounded by
 * max_pending_directories; roots are processed in input order. */
struct CatalogScanOptions {
  std::size_t max_entries{10000};
  std::size_t max_pending_directories{256};
  std::string search;
  std::function<void(CatalogScanProgress)> progress;
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

/** Recursively scan all roots, suppressing duplicate files by normalized
 * absolute path and preserving the first root's result. Missing or
 * disappearing roots are returned as per-root filesystem errors. Results are
 * empty when cancellation or the global entry budget is observed. */
CatalogScanResult scan_roots(const std::vector<std::filesystem::path>& roots,
                             CatalogScanOptions options = {},
                             std::stop_token stop = {});

} // namespace beatglass::library
