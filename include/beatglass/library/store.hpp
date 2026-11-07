#pragma once
#include "beatglass/core/results.hpp"
#include "beatglass/library/jobs.hpp"
#include <cstdint>
#include <filesystem>
#include <optional>
#include <stop_token>
#include <string>
namespace beatglass {
struct FileVersion { std::filesystem::path path; std::uint64_t size_bytes{}; std::int64_t modified_ns{}; std::string content_hash; };
struct CachedAnalysis { AnalysisIdentity identity; Snapshot result; };
// Worker-owned interface. Implementations serialize writes and transact the
// replacement only after rechecking file version and successful completion.
// Cancelled or failed runs never replace an existing completed result.
class AnalysisStore {
public:
  virtual ~AnalysisStore() = default;
  virtual std::optional<CachedAnalysis> find(const FileVersion&, const AnalysisIdentity&, std::stop_token) = 0;
  virtual bool publish(const FileVersion& before, const FileVersion& after,
                       const CachedAnalysis&, std::stop_token) = 0;
};
}
