#pragma once
#include "beatglass/library/detail/file_job.hpp"
#include "beatglass/library/store.hpp"
#include <optional>
namespace beatglass::library_detail {
/** Build the exact tempo-only cache identity used by the snapshot job. The content digest must be validated separately; engine and configuration must be nonempty. */
AnalysisIdentity snapshot_tempo_identity(const std::string& content, const std::string& engine, const std::string& configuration);
/** Outcome of the experimental shared-store snapshot job. */
struct StoredSnapshotJobResult {
  /** Terminal job state. */ FileJobStatus status{FileJobStatus::error};
  /** Cached or committed tempo-only snapshot, when available. */ std::optional<CachedAnalysis> value;
  /** Diagnostic for errors. */ std::string diagnostic;
  /** Run ID to query after an indeterminate storage error. */ std::string recovery_run_id;
};
/** Run snapshot-backed real tempo analysis through a worker-owned AnalysisStore. The store and verifier are used synchronously on this worker. The job appends a causal-tempo algorithm version to the caller's engine version for cache identity. The caller supplies an epoch allocated uniquely for this analysis session; the job does not allocate epochs. Cached frame epochs are rebound to that epoch and publication monotonic time is refreshed. Returns only tempo snapshot fields; no key/chord or beat history is produced. This is an experimental integration, not wired to UI or product, and retains the documented source-verification and snapshot races. */
StoredSnapshotJobResult analyze_snapshot_to_store(AnalysisStore& store, const std::filesystem::path& path,
  const std::string& engine_version, const std::string& configuration_id, std::uint64_t epoch,
  std::stop_token stop = {}, const std::function<void(std::uint64_t)>& progress = {});
}
