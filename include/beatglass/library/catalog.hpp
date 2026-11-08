#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace beatglass::library {

/** One visible entry in a local library folder.
 * The path is absolute when the input folder is absolute, and the entry does
 * not own or modify the referenced file. Directories are included so callers
 * can navigate; regular files are included only for the qualified WAV, FLAC,
 * MP3, and AIFF v1 formats.
 */
struct CatalogEntry {
  std::filesystem::path path;
  std::string name;
  bool directory{};
};

/** Result of a synchronous folder query performed by a non-audio worker.
 * On filesystem failure, entries is empty and diagnostic contains the
 * platform error. The query never follows directory entries recursively.
 */
struct CatalogResult {
  std::vector<CatalogEntry> entries;
  std::string diagnostic;
};

/** Enumerate one local folder for the Library view.
 * The search is case-insensitive against entry names. Results are sorted by
 * case-insensitive name, then exact name, then path for deterministic UI and
 * test behavior. This function performs filesystem I/O and must not run in an
 * audio callback; it does not create, delete, or modify user files.
 */
CatalogResult browse(const std::filesystem::path& folder,
                     std::string search = {});

} // namespace beatglass::library
