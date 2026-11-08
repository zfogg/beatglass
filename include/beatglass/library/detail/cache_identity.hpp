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

/** Refresh one regular-file catalog row from a validated content observation.
 * A changed, cancelled, or failed fingerprint leaves the previous row intact.
 */
FingerprintStatus refresh_collection_file(SqliteCache& cache, const std::filesystem::path& path,
                                          std::stop_token stop = {},
                                          std::string* diagnostic = nullptr);
/** Recompute a file fingerprint and compare it with a previously published key.
 * Returns `ok` only when the observed digest equals expected_content; callers
 * should treat every other result as a cache miss or stale-input outcome.
 */
FingerprintStatus revalidate_file_identity(const std::filesystem::path& path,
                                           const std::string& expected_content,
                                           std::stop_token stop = {},
                                           std::string* diagnostic = nullptr);
}
