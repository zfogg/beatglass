#pragma once
#include "beatglass/library/storage_config.hpp"
#include "beatglass/library/detail/sqlite_cache.hpp"
#include <memory>
#include <string>
#include <vector>
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
  /** Read persisted retryable runs on the storage worker; returns empty while closed. */
  std::vector<library_detail::RetryableRun> retryable_runs(std::size_t limit = 100) const;
  /** Discard one retryable run on the storage worker; never removes source audio. */
  bool discard_retryable(const std::string& id);
private:
  std::unique_ptr<library_detail::SqliteCache> cache_;
};
}
