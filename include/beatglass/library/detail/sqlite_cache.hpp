#pragma once
#include <cstddef>
#include <functional>
#include <memory>
#include <cstdint>
#include <optional>
#include <vector>
#include <stop_token>
#include <string>
namespace beatglass::library_detail {
/** Return whether a byte length fits SQLite's signed-int binding APIs. */
bool sqlite_length_representable(std::size_t size) noexcept;
/** Provisional caller-supplied cache identity. The caller must establish a validated content hash and stable algorithm/configuration versions. */
struct CacheKey {
  /** Validated content identity. */ std::string content;
  /** Engine version. */ std::string engine;
  /** Configuration identity. */ std::string config;
  /** Analysis mode. */ std::string mode;
};
/** Provisional publication outcome; error may include an indeterminate commit. */
enum class Publish {
  /** Active result was replaced. */ committed,
  /** Cancellation was observed before commit. */ cancelled,
  /** The caller's revalidation rejected the input. */ stale_input,
  /** A storage operation failed; a failed commit may require recovery by run ID. */ error
};
/** Persisted state of a unique run, queried after reopening to resolve an uncertain commit. */
enum class RunRecovery {
  /** No run with that ID and key exists. */ missing,
  /** The run did not commit and may be retried under a new ID. */ retryable,
  /** The run committed, even if a later run is now active. */ committed
};
/** Availability recorded by the indexing worker. */
enum class EntryAvailability {
  /** The last index operation observed the path. */ present,
  /** The last index operation could not find the path. */ missing,
  /** The last index operation failed for another reason. */ error
};
/** Owned indexed metadata; the UTF-8 path is a caller-normalized key. A nonempty content hash is caller-supplied evidence, not proof that the path is still unchanged. */
struct CollectionEntry {
  /** Normalized UTF-8 path key; never interpreted as a delete request. */ std::string path;
  /** Optional title; empty means unknown. */ std::string title;
  /** Optional artist; empty means unknown. */ std::string artist;
  /** Validated content identity if available; empty means unknown. */ std::string content_hash;
  /** Last observed source byte size. */ std::uint64_t size_bytes{};
  /** Caller-defined high-resolution modification evidence; clock semantics need integration approval. */ std::int64_t modified_ns{};
  /** Decoded frame count, if known. */ std::optional<std::uint64_t> frames;
  /** Decoded frames per second, if known. */ std::optional<std::uint32_t> sample_rate;
  /** Last observed availability. */ EntryAvailability availability{EntryAvailability::present};
  /** Optional diagnostic from the indexing worker. */ std::string diagnostic;
};
/** Whitelisted collection ordering. */
enum class CollectionOrder { /** Path key. */ path, /** Title with path tie-break. */ title, /** Artist with path tie-break. */ artist };
/** Bounded literal-search query; text matching and case-insensitive ordering use SQLite's ASCII-only lower/NOCASE behavior. Pages can shift if the collection changes between queries. */
struct CollectionQuery {
  /** Literal search across path, title and artist. */ std::string search;
  /** Sort field. */ CollectionOrder order{CollectionOrder::path};
  /** Reverse the selected sort field. */ bool descending{};
  /** Maximum rows, 1 through 200. */ std::uint32_t limit{100};
  /** Offset into the current ordering. */ std::uint32_t offset{};
};
/** Provisional worker-owned SQLite cache. Not thread-safe; construct, use, and destroy on one worker. A file-backed instance holds an exclusive SQLite lock for its lifetime, so other connections fail rather than misclassifying active runs as interrupted. No input files are modified. */
class SqliteCache {
public:
  /** Open or migrate a database and recover unfinished runs; throws on errors or unsupported schema. */
  explicit SqliteCache(const std::string& path);
  /** Close the connection. */ ~SqliteCache();
  SqliteCache(const SqliteCache&) = delete;
  SqliteCache& operator=(const SqliteCache&) = delete;
  /** Insert or replace one index row on the worker; throws on invalid values or storage errors. Does not touch the source file. */
  void put_entry(const CollectionEntry& entry);
  /** Remove only the index row; returns whether a row existed. Never removes a source file. */
  bool remove_entry(const std::string& path);
  /** Return an owned exact-path entry or absence; throws on storage errors. */
  std::optional<CollectionEntry> entry(const std::string& path);
  /** Return up to the requested bounded page of owned entries; throws on invalid query or storage errors. */
  std::vector<CollectionEntry> entries(const CollectionQuery& query);
  /** Insert a uniquely identified running job; throws on SQLite error or duplicate ID. */
  void begin(const std::string& id, const CacheKey& key);
  /** Copy a completed cached blob into result; return false on miss, throw on storage error. */
  bool find(const CacheKey& key, std::string& result);
  /** Recover a run outcome after reopening the database. The ID and all key fields must match. Throws on SQLite error. Reopening converts interrupted running rows to retryable; a completed row remains evidence of commit even if superseded. */
  RunRecovery recover(const std::string& id, const CacheKey& key);
  /** Query whether a unique run ID is committed, or not committed; throws on storage error. */
  bool committed_run(const std::string& id);
  /** Revalidate and atomically publish. Cancellation before commit rolls back; cancellation after commit begins does not override its outcome. A callback must validate the opened source version; stat/hash checks have filesystem races. The optional test probe runs immediately after successful COMMIT; throwing simulates a lost acknowledgement and returns error, which requires run-ID recovery. It must not reenter the cache. */
  Publish publish(const std::string& id, const CacheKey& key, const std::string& result,
                  std::stop_token stop, const std::function<bool()>& revalidate,
                  const std::function<void()>& after_commit_probe = {});
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
