#include "beatglass/library/storage_owner.hpp"
namespace beatglass::library {
bool StorageOwner::open(const StorageConfig& config, std::string* diagnostic) {
  close();
  if (!valid_storage_config(config, diagnostic)) return false;
  try { cache_ = std::make_unique<library_detail::SqliteCache>(config.database_path.string()); return true; }
  catch (const std::exception& e) { if (diagnostic) *diagnostic = e.what(); return false; }
}
void StorageOwner::close() noexcept { cache_.reset(); }
}
