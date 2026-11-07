#pragma once
#include "beatglass/core/file_analysis.hpp"
#include "beatglass/library/detail/sqlite_cache.hpp"
#include <filesystem>
#include <functional>
#include <optional>
#include <stop_token>
#include <string>
namespace beatglass::library_detail {
/** Terminal state of the provisional tempo cache job. */
enum class FileJobStatus {
  /** Existing completed analysis was reused. */ cache_hit,
  /** A new analysis was committed. */ committed,
  /** Cancellation was observed before commit. */ cancelled,
  /** Input evidence changed during the operation. */ stale_input,
  /** Input, analysis, result or storage failed; diagnostics are available. */ error
};
/** Result copied from the database or analyzer. */
struct FileJobResult {
  /** Terminal state. */ FileJobStatus status{FileJobStatus::error};
  /** Tempo data for a cache hit or committed job. */ std::optional<TempoResult> result;
  /** Diagnostic for error outcomes. */ std::string diagnostic;
};
/** Analyze or reuse a file's provisional tempo result on the calling database worker. Hashes the whole file before lookup and after analysis; progress reports decoded frames synchronously. Cancellation is cooperative between decoded blocks and hash chunks, not during blocking I/O. The decoder separately opens the path, so replacement/ABA races remain; this is not yet suitable for product publication. The source file is never written. */
FileJobResult analyze_cached_tempo(SqliteCache& cache, const std::filesystem::path& path,
  const std::string& engine_version, const std::string& configuration_id,
  std::stop_token stop = {}, const std::function<void(std::uint64_t)>& progress = {});
}
