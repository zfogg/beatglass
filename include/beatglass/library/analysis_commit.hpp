#pragma once
#include "beatglass/library/analysis_operation.hpp"
#include "beatglass/library/detail/sqlite_cache.hpp"
namespace beatglass::library {
/** Look up a completed result for an operation identity before running analysis. Returns false on a cache miss. Calls remain on the owning worker thread. */
bool reuse_cached_operation(library_detail::SqliteCache&, const AnalysisIdentity&, std::string& result);
/**
 * Publish one completed operation through the worker-owned cache boundary.
 * Decoder operations are executed with the caller's stop token before the
 * transactional commit; serialization operations remain supported. The run
 * identity is retained by the cache for retryable/cancelled publication.
 * Invalid operations return an error without touching storage. When a run has
 * already been begun, cancellation or serialization failure before commit
 * marks that run retryable; it never masquerades as a completed cache hit.
 * Calls and the cache must remain on the owning worker thread.
 */
library_detail::Publish publish_operation(library_detail::SqliteCache&, const std::string& run_id,
                                           const AnalysisOperation&, std::stop_token);
/** Create a stop-aware transactional publisher carrying run identity and revalidation. */
std::function<bool(const std::string&, std::stop_token)> make_transactional_publisher(
    library_detail::SqliteCache&, std::string run_id, AnalysisIdentity identity,
    std::function<FileVerificationResult(std::stop_token)> verify);
}
