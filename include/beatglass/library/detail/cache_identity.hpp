#pragma once
#include "beatglass/library/detail/file_fingerprint.hpp"
#include "beatglass/library/detail/sqlite_cache.hpp"
#include <filesystem>
#include <stop_token>
#include <string>
namespace beatglass::library_detail {
/** Result of deriving a cache key from one bounded content observation. */
struct CacheIdentityResult {
  FingerprintStatus status{FingerprintStatus::error};
  CacheKey key{};
  std::string diagnostic;
};
/**
 * Fingerprint a path and construct the cache identity used by publication.
 * Only an `ok` result may be passed to `SqliteCache`; changed, cancelled, or
 * failed observations never become cache keys. The caller must still provide
 * the same source revalidation callback when publishing the completed result.
 */
CacheIdentityResult cache_identity_from_file(const std::filesystem::path& path,
                                             std::string engine,
                                             std::string config,
                                             std::string mode,
                                             std::stop_token stop = {});
}
