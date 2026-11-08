#pragma once
#include "beatglass/library/storage_config.hpp"
#include "beatglass/library/detail/sqlite_cache.hpp"
#include <memory>
#include <string>
namespace beatglass::library {
/** Worker-owned SQLite storage lifetime. Construct, open, use, and destroy on one database worker; never call from an audio callback or GUI thread. */
class StorageOwner final {
public:
  StorageOwner() = default;
  StorageOwner(const StorageOwner&) = delete;
  StorageOwner& operator=(const StorageOwner&) = delete;
  /** Validate configuration and open storage; failure leaves the owner closed. */
  bool open(const StorageConfig& config, std::string* diagnostic = nullptr);
  /** Close storage on the owning worker. */
  void close() noexcept;
  /** Return the open cache, or null while closed. */
  library_detail::SqliteCache* cache() noexcept { return cache_.get(); }
private:
  std::unique_ptr<library_detail::SqliteCache> cache_;
};
}
