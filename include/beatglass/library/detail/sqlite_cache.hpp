#pragma once
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <stop_token>
#include <string>
#include <vector>
namespace beatglass::library_detail {
/** On-disk schema version for the provisional analysis cache. */
inline constexpr int kCacheSchemaVersion = 1;
/** Metadata for one indexed local-library item; the source file is never owned or modified. */
struct CollectionEntry {
  std::string path, name, kind, fingerprint;
  std::uint64_t size_bytes{};
  std::int64_t modified_ns{};
};
/** Bounded, case-insensitive name query over indexed entries. */
/** Case-insensitive name-prefix page; offset is zero-based and limit must be between 1 and 10000. */
struct CollectionQuery { std::string search; std::size_t offset{}; std::size_t limit{100}; };
/** Return whether a byte length fits SQLite's signed-int binding APIs. */
bool sqlite_length_representable(std::size_t size) noexcept;
/** Maximum serialized analysis result accepted by the provisional cache. */
inline constexpr std::size_t kMaxCachedResultBytes = 8u * 1024u * 1024u;
/** Provisional caller-supplied cache identity. The caller must establish a validated content hash and stable algorithm/configuration versions. */
struct CacheKey {
  /** Validated content identity. */ std::string content;
  /** Engine version. */ std::string engine;
  /** Configuration identity. */ std::string config;
  /** Analysis mode. */ std::string mode;
};
/** Persisted analysis run that can be resumed after a process restart. */
struct RetryableRun { std::string id; CacheKey key; };
/** Provisional publication outcome; error may include an indeterminate commit. */
enum class Publish {
  /** Active result was replaced. */ committed,
  /** Cancellation was observed before commit. */ cancelled,
  /** The caller's revalidation rejected the input. */ stale_input,
  /** A storage operation failed; a failed commit may require recovery by run ID. */ error
};
/** Provisional worker-owned SQLite cache. Not thread-safe; construct, use, and destroy on one worker. A file-backed instance holds an exclusive SQLite lock for its lifetime, so other connections fail rather than misclassifying active runs as interrupted. No input files are modified. */
class SqliteCache {
public:
  /** Open or migrate a database and recover unfinished runs; throws on errors or unsupported schema. */
  explicit SqliteCache(const std::string& path);
  /** Close the connection. */ ~SqliteCache();
  SqliteCache(const SqliteCache&) = delete;
  SqliteCache& operator=(const SqliteCache&) = delete;
  /** Insert or replace one index row on the worker; throws on empty, embedded-NUL, or oversized metadata and storage errors. Does not touch the source file. */
  void put_entry(const CollectionEntry& entry);
  /** Remove only the index row; rejects an empty or embedded-NUL path and returns whether a row existed. Never removes a source file. */
  bool remove_entry(const std::string& path);
  /** Return an owned exact-path entry or absence; throws CollectionCancelled on observed stop and throws on storage errors. */
  std::optional<CollectionEntry> entry(const std::string& path, std::stop_token stop = {});
  /** Return up to the requested bounded page of owned entries; throws CollectionCancelled on observed stop and throws on invalid query or storage errors. */
  std::vector<CollectionEntry> entries(const CollectionQuery& query, std::stop_token stop = {});
  /** Insert a uniquely identified running job; throws on invalid identity, invalid run ID, SQLite error, or duplicate ID. */
  void begin(const std::string& id, const CacheKey& key);
  /** Copy a completed cached blob into result; return false on miss, throw on storage error. */
  bool find(const CacheKey& key, std::string& result);
  /** Return 1–10000 persisted runs left retryable by cancellation, stale input, or restart. */
  std::vector<RetryableRun> retryable_runs(std::size_t limit = 100);
  /** Discard one retryable run record without touching its source file; throws for an empty, NUL-containing, or overlong ID. */
  bool discard_retryable(const std::string& id);
  /** Mark a matching running operation retryable after a pre-publication worker failure; returns false when ID/key do not match a running row and throws for invalid inputs. */
  bool mark_retryable(const std::string& id, const CacheKey& key);
  /** Remove the active result for an exact identity; returns false when none exists, throws for an invalid key, and leaves source files and historical runs untouched. */
  bool invalidate(const CacheKey& key);
  /** Revalidate and atomically publish. Cancellation before commit rolls back; cancellation after commit begins does not override its outcome. A callback must validate the opened source version; stat/hash checks have filesystem races. */
  Publish publish(const std::string& id, const CacheKey& key, const std::string& result,
                  std::stop_token stop, const std::function<bool()>& revalidate);
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
