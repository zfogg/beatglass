#pragma once
#include "beatglass/library/storage_config.hpp"
#include "beatglass/library/detail/sqlite_cache.hpp"
#include "beatglass/library/jobs.hpp"
#include "beatglass/library/analysis_commit.hpp"
#include "beatglass/library/analysis_admission.hpp"
#include <stop_token>
#include <optional>
#include <memory>
#include <string>
#include <vector>
namespace beatglass::library {
/** Worker-owned SQLite storage lifetime. Construct, open, use, and destroy on one database worker; never call from an audio callback or GUI thread. */
class StorageOwner final {
public:
  StorageOwner() = default;
  StorageOwner(const StorageOwner&) = delete;
  StorageOwner& operator=(const StorageOwner&) = delete;
  /** Validate configuration and open storage; failure leaves the owner closed. */
  bool open(const StorageConfig& config, std::string* diagnostic = nullptr);
  /** Close storage on the owning worker. */
  void close() noexcept;
  /** Return the open cache, or null while closed. */
  library_detail::SqliteCache* cache() noexcept { return cache_.get(); }
  /** Find a completed cached result by identity on the storage worker. Returns false while closed or on a miss. */
  bool find_cached(const AnalysisIdentity& identity, std::string& result) const;
  /** Invalidate one completed identity on the storage worker; never removes source files or retry records. */
  bool invalidate_cached(const AnalysisIdentity& identity);
  /** Publish a verified decoder/analysis result through the open cache, or return an error while closed. */
  library_detail::Publish publish_analysis(const std::string& run_id, const AnalysisOperation& operation,
                                           std::stop_token stop = {});
  /** Reuse a completed result or reserve run_id before worker decoding; closed storage returns an empty admission. */
  CacheAdmission admit_analysis(const std::string& run_id, const AnalysisIdentity& identity, const std::string& source_path = {}, const std::string& decoder_snapshot = {});
  /** Read persisted retryable runs on the storage worker; returns empty while closed. */
  std::vector<library_detail::RetryableRun> retryable_runs(std::size_t limit = 100) const;
  /** Read one persisted retryable run by ID, including source/decoder metadata; empty while closed or absent. */
  std::optional<library_detail::RetryableRun> retryable_run(const std::string& id) const;
  /** Discard one retryable run on the storage worker; never removes source audio. */
  bool discard_retryable(const std::string& id);
private:
  std::unique_ptr<library_detail::SqliteCache> cache_;
};
}
