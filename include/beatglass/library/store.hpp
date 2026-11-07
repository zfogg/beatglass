#pragma once
#include "beatglass/core/results.hpp"
#include "beatglass/library/jobs.hpp"
#include <cstdint>
#include <filesystem>
#include <optional>
#include <stop_token>
#include <string>
namespace beatglass {
/// File identity evidence: path, size, modification time, and content hash. Hash policy remains unspecified.
struct FileVersion { std::filesystem::path path; std::uint64_t size_bytes{}; std::int64_t modified_ns{}; std::string content_hash; };
/// Cached identity and value snapshot.
struct CachedAnalysis { AnalysisIdentity identity; Snapshot result; };
// Worker-owned interface. Implementations serialize writes and transact the
// replacement only after rechecking file version and successful completion.
// Cancelled or failed runs never replace an existing completed result.
/// Provisional worker-owned storage boundary, not a database implementation. Implementations must serialize writes, recheck file version and transact replacement only after success. Cancellation or failure must not replace a completed result. Error and cancellation reporting remain unspecified.
class AnalysisStore {
public:
  virtual ~AnalysisStore() = default;
/// Look up a cached result for a file version and analysis identity, with a cancellation token.
  virtual std::optional<CachedAnalysis> find(const FileVersion&, const AnalysisIdentity&, std::stop_token) = 0;
/// Request publication using before and after file versions and a cancellation token; detailed return semantics remain unspecified.
  virtual bool publish(const FileVersion& before, const FileVersion& after,
                       const CachedAnalysis&, std::stop_token) = 0;
};
}
