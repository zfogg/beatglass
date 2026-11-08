#pragma once
#include "beatglass/library/detail/sqlite_cache.hpp"
#include "beatglass/library/jobs.hpp"
#include <optional>
#include <string>
namespace beatglass::library {
/** Worker-owned cache admission result. A hit contains a completed serialized result; a miss reserves run_id for analysis. */
struct CacheAdmission { std::optional<std::string> cached; bool analysis_reserved{}; };
/** Reuse a completed identity or reserve a retryable run before decoding/analysis begins. */
CacheAdmission admit_analysis(library_detail::SqliteCache&, const std::string& run_id, const AnalysisIdentity&, const std::string& source_path = {}, const std::string& decoder_snapshot = {});
}
