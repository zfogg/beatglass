#pragma once
#include "beatglass/library/analysis_operation.hpp"
#include "beatglass/library/detail/sqlite_cache.hpp"
namespace beatglass::library {
/**
 * Publish one completed operation through the worker-owned cache boundary.
 * Invalid operations return an error without touching storage. When a run has
 * already been begun, cancellation or serialization failure before commit
 * marks that run retryable; it never masquerades as a completed cache hit.
 * Calls and the cache must remain on the owning worker thread.
 */
library_detail::Publish publish_operation(library_detail::SqliteCache&, const std::string& run_id,
                                           const AnalysisOperation&, std::stop_token);
}
