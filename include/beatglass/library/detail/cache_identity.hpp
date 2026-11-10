#pragma once
#include "beatglass/library/detail/file_fingerprint.hpp"
#include "beatglass/library/detail/sqlite_cache.hpp"
#include <filesystem>
#include <stop_token>
#include <string>
namespace beatglass::library_detail {
/** Result of deriving a cache key from one bounded content observation. */
struct CacheIdentityResult {
  /** Fingerprint outcome; only ok makes key valid for publication. */
  FingerprintStatus status{FingerprintStatus::error};
  /** Content/configuration identity; unspecified when status is not ok. */
  CacheKey key{};
  /** Stable status-prefixed diagnostic for non-ok outcomes. */
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
 * The optional diagnostic is cleared on entry and receives a stable status
 * label plus detail when observation does not succeed.
 */
FingerprintStatus refresh_collection_file(SqliteCache& cache, const std::filesystem::path& path,
                                          std::stop_token stop = {},
                                          std::string* diagnostic = nullptr);
/** Recompute a file fingerprint and compare it with a previously published key.
 * Returns `ok` only when the observed digest equals expected_content; callers
 * should treat every other result as a cache miss or stale-input outcome. The
 * optional diagnostic is cleared on entry and is safe to display to a user.
 */
FingerprintStatus revalidate_file_identity(const std::filesystem::path& path,
                                           const std::string& expected_content,
                                           std::stop_token stop = {},
                                           std::string* diagnostic = nullptr);
/** Revalidate a cached identity and remove its active result only when stale.
 * Matching, cancelled, and unreadable observations leave the active row intact;
 * callers can distinguish the outcome and choose whether to retry or surface it.
 */
FingerprintStatus invalidate_if_stale(SqliteCache& cache, const CacheKey& key,
                                      const std::filesystem::path& path,
                                      std::stop_token stop = {},
                                      std::string* diagnostic = nullptr);
/** Begin/publish one file-backed result only after an initial identity check.
 * The cache publication callback repeats revalidation at the commit point;
 * changed input becomes `stale_input` and remains retryable.
 */
Publish publish_file_result(SqliteCache& cache, const std::string& run_id,
                            const CacheKey& key, const std::filesystem::path& path,
                            const std::string& result, std::stop_token stop = {});
/** Revalidate a catalog file and remove its row only when the path is stale.
 * A matching, cancelled, or unreadable path leaves the row available for
 * retry/diagnostics; changed content removes it and reports why. Missing
 * paths are pruned with a diagnostic when a row was removed.
 */
FingerprintStatus invalidate_collection_if_stale(SqliteCache& cache, const std::string& path,
                                                 const std::string& expected_content,
                                                 std::stop_token stop = {},
                                                 std::string* diagnostic = nullptr);
/** Remove a catalog row when its source path no longer exists.
 * Returns true only when a row was removed; filesystem errors leave the row
 * intact so callers can retry or surface the diagnostic.
 */
bool prune_missing_collection_entry(SqliteCache& cache, const std::filesystem::path& path,
                                    std::string* diagnostic = nullptr);
}
