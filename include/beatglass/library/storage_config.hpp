#pragma once
#include <filesystem>
#include <string>
namespace beatglass::library {
/**
 * Explicit caller-owned storage configuration; no platform default is
 * selected. The parent directory is not created or owned by this value.
 */
struct StorageConfig { std::filesystem::path database_path; };
/**
 * Validate configuration before constructing worker-owned SQLite storage.
 * This is structural validation only: it requires a nonempty path naming a
 * file, but does not resolve the path, create parent directories, or probe
 * permissions. SQLite open errors are reported by `StorageOwner::open`.
 */
inline bool valid_storage_config(const StorageConfig& config, std::string* diagnostic = nullptr) {
  if (config.database_path.empty()) { if (diagnostic) *diagnostic = "database path is required"; return false; }
  if (config.database_path.filename().empty()) { if (diagnostic) *diagnostic = "database path must name a file"; return false; }
  return true;
}
}
