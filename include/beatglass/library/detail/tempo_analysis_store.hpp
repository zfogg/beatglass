#pragma once
#include "beatglass/library/store.hpp"
#include "beatglass/library/detail/sqlite_cache.hpp"
#include <memory>
#include <string>
namespace beatglass::library_detail {
/** Worker-confined, provisional SQLite AnalysisStore adapter for tempo-only snapshots. Not thread-safe or installed. Its versioned serialization namespace is distinct from the earlier private tempo-result blob. It rejects candidate, beat and chord collections rather than silently discarding them. Input identity must already be validated by the caller; publication invokes the supplied verifier. */
class TempoAnalysisStore final : public AnalysisStore {
public:
  /** Open the exclusively owned database; may block and throws on storage errors. */
  explicit TempoAnalysisStore(const std::string& path);
  /** Close on the owning worker after all operations finish. */ ~TempoAnalysisStore() override;
  TempoAnalysisStore(const TempoAnalysisStore&) = delete;
  TempoAnalysisStore& operator=(const TempoAnalysisStore&) = delete;
  /** Index or update metadata on the owning worker; no source file is modified. */
  void put_entry(const CollectionEntry& entry);
  /** Delete only an index row, never the user's source file. */
  bool remove_entry(const std::string& path);
  /** Copy one exact-path metadata row, if present. */
  std::optional<CollectionEntry> entry(const std::string& path, std::stop_token stop = {});
  /** Copy a bounded metadata page; queries remain on the owning worker. */
  std::vector<CollectionEntry> entries(const CollectionQuery& query, std::stop_token stop = {});
  /** Look up a validated identity; returns cancellation, miss, hit, or storage/format error. */
  FindResult find(const FileVersion&, const AnalysisIdentity&, std::stop_token) override;
  /** Stage and atomically publish after verifier approval. The current shared verifier cannot observe a stop token while blocking. */
  PublishResult publish(const FileVersion&, const CachedAnalysis&, const std::string&, FileVersionVerifier&, std::stop_token) override;
  /** Query a run ID after reopening; a non-completed run maps to not_found. */
  RecoveryResult recover(const std::string&) override;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
