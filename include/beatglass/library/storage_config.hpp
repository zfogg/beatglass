#pragma once
#include <filesystem>
#include <string>
namespace beatglass::library {
/** Explicit caller-owned storage configuration; no platform default is selected. */
struct StorageConfig { std::filesystem::path database_path; };
/** Validate configuration before constructing worker-owned SQLite storage. */
inline bool valid_storage_config(const StorageConfig& config, std::string* diagnostic = nullptr) {
  if (config.database_path.empty()) { if (diagnostic) *diagnostic = "database path is required"; return false; }
  if (config.database_path.filename().empty()) { if (diagnostic) *diagnostic = "database path must name a file"; return false; }
  return true;
}
}
