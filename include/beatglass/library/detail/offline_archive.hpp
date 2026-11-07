#pragma once
#include "beatglass/core/offline_analysis.hpp"
#include "beatglass/library/detail/sqlite_cache.hpp"
#include "beatglass/library/store.hpp"
namespace beatglass::library_detail {
/** Exact content and algorithm identity for one offline result. Algorithm entries must be sorted uniquely by name. */
struct OfflineKey {
  std::string content_hash, engine_version, configuration_id; ///< Validated content and full configuration/version identity.
  OfflineMode mode{OfflineMode::causal}; ///< Processing mode.
  std::vector<OfflineAlgorithmVersion> algorithms; ///< Canonically ordered component identities.
};
/** Owned page from one immutable completed run; the header has empty event vectors. */
struct OfflinePage {
  std::string run_id; ///< Immutable completed run identifier.
  OfflineAnalysis header; ///< Header rebound to the caller's session epoch.
  std::vector<OfflineBeat> beats; ///< Requested bounded beat slice.
  std::vector<OfflineTonalWindow> windows; ///< Requested bounded window slice.
  std::size_t next_beat{}, next_window{}; ///< Next offsets; equal to totals when exhausted.
  std::size_t total_beats{}, total_windows{}; ///< Counts in this immutable run.
};
/** Typed lookup or page status; changed includes unverifiable source evidence. */
enum class OfflineReadStatus { hit, miss, changed, cancelled, error };
/** Owned read result. */
struct OfflineReadResult {
  OfflineReadStatus status{OfflineReadStatus::error}; ///< Terminal read outcome.
  std::optional<OfflinePage> page; ///< Page only on hit.
  std::string diagnostic; ///< Optional error explanation.
};
/** Worker-confined private offline storage experiment using the same SqliteCache connection. Not a product API; source verification remains subject to platform races. Callers must bind source identity and verifier to the same opened snapshot and independently establish actual loaded runtime and configuration identity. Non-cacheable producer results must not be published. */
class OfflineArchive {
public:
  /** Borrow a cache on its owning worker; it must outlive this archive. */
  explicit OfflineArchive(SqliteCache& cache) : cache_(cache) {}
  /** Validate and atomically publish one bounded offline value and existing index row. A successful commit wins over late cancellation; unknown commit outcomes require run recovery. */
  PublishResult publish(const FileVersion& source, const OfflineKey& key, const OfflineAnalysis& value, const std::string& run, const std::string& index_path, FileVersionVerifier& verifier, std::stop_token stop);
  /** Revalidate and return an owned page from the exact immutable run and key, or use an empty run ID to resolve the current active run. Each requested count is at most 200; the full encoded value is bounded to 8 MiB and is decoded on every request, with additional bounded vector allocations. A supplied run must still be active or changed is returned. Epoch must be nonzero and is rebound on return. */
  OfflineReadResult page(const FileVersion& source, const OfflineKey& key, const std::string& run, std::size_t beat_offset, std::size_t beat_count, std::size_t window_offset, std::size_t window_count, std::uint64_t epoch, FileVersionVerifier& verifier, std::stop_token stop);
private:
  SqliteCache& cache_;
};
}
