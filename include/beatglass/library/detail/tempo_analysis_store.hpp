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
  /** Close on the owning worker after all operations finish. */ ~TempoAnalysisStore();
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
  /** Borrow the underlying worker-confined cache for the provisional offline archive; it must not outlive this store. */
  SqliteCache& offline_cache();
  /** Look up a validated identity; returns cancellation, miss, hit, or storage/format error. */
  FindResult find(const FileVersion&, const AnalysisIdentity&, std::stop_token) override;
  /** Stage and atomically publish after verifier approval. Passes the stop token to the verifier; an in-flight blocking I/O operation may not be interruptible. */
  PublishResult publish(const FileVersion&, const CachedAnalysis&, const std::string&, FileVersionVerifier&, std::stop_token) override;
  /** Publish and update the existing indexed row in the same transaction; failure preserves the prior active result and row. */
  PublishResult publish_indexed(const FileVersion&, const CachedAnalysis&, const std::string& run, const std::string& path, FileVersionVerifier&, std::stop_token);
  /** Verify and associate an existing active identity with an indexed row transactionally. */
  PublishResult associate(const FileVersion&, const AnalysisIdentity&, const std::string& path, FileVersionVerifier&, std::stop_token);
  /** Query a run ID after reopening; a non-completed run maps to not_found. */
  RecoveryResult recover(const std::string&) override;
private:
  PublishResult publish_impl(const FileVersion&, const CachedAnalysis&, const std::string&, const std::string*, FileVersionVerifier&, std::stop_token);
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
