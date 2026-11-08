#pragma once
#include "beatglass/library/analysis_operation.hpp"
#include "beatglass/library/detail/sqlite_cache.hpp"
namespace beatglass::library { /** Publish one completed operation through the worker-owned cache boundary; invalid or cancelled operations are rejected before cache publication or source revalidation. */ library_detail::Publish publish_operation(library_detail::SqliteCache&, const std::string& run_id, const AnalysisOperation&, std::stop_token); }
