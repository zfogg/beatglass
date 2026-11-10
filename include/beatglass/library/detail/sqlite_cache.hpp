#pragma once
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <stop_token>
#include <string>
#include <vector>
#include <stdexcept>
namespace beatglass::library_detail {
/** Raised when a collection read observes cancellation before or during SQLite iteration. */
class CollectionCancelled : public std::runtime_error {
public:
  CollectionCancelled() : std::runtime_error("collection query cancelled") {}
};
/** On-disk schema version for the provisional analysis cache. */
inline constexpr int kCacheSchemaVersion = 2;
/** Metadata for one indexed local-library item; the source file is never owned or modified. */
struct CollectionEntry {
  std::string path, name, kind, fingerprint;
  std::uint64_t size_bytes{};
  std::int64_t modified_ns{};
};
/** Bounded, case-insensitive name query over indexed entries. */
/** Case-insensitive name-prefix page; offset is zero-based and limit must be between 1 and 10000. */
inline constexpr std::size_t kMaxCollectionSearchBytes = 1024;
inline constexpr std::size_t kMaxCollectionPageSize = 10000;
inline constexpr std::size_t kMaxCollectionOffset = static_cast<std::size_t>(0x7fffffffffffffffULL);
struct CollectionQuery { std::string search; std::size_t offset{}; std::size_t limit{100}; }; /** Search prefixes are limited to kMaxCollectionSearchBytes. */
/** Return whether a byte length fits SQLite's signed-int binding APIs. */
bool sqlite_length_representable(std::size_t size) noexcept;
/** Maximum serialized analysis result accepted by the provisional cache. */
inline constexpr std::size_t kMaxCachedResultBytes = 8u * 1024u * 1024u;
inline constexpr std::size_t kMaxRetryableRuns = 10000;
inline constexpr std::size_t kMaxCacheRunIdBytes = 4096;
inline constexpr std::size_t kMaxCacheContentIdentityBytes = 512;
inline constexpr std::size_t kMaxCacheEngineVersionBytes = 4096;
inline constexpr std::size_t kMaxCacheConfigurationBytes = 4096;
inline constexpr std::size_t kMaxCacheModeBytes = 128;
inline constexpr std::size_t kMaxCollectionFingerprintBytes = 4096;
inline constexpr std::size_t kMaxCollectionNameBytes = 1024;
inline constexpr std::size_t kMaxCollectionKindBytes = 128;
inline constexpr std::size_t kMaxCollectionPathBytes = 4096;
/** Provisional caller-supplied cache identity. The caller must establish a validated content hash and stable algorithm/configuration versions. */
struct CacheKey {
  /** Validated content identity. */ std::string content;
  /** Engine version. */ std::string engine;
  /** Configuration identity. */ std::string config;
  /** Analysis mode. */ std::string mode;
};
/** Persisted analysis run that can be resumed after a process restart. */
/** Persisted retry input snapshot; empty decoder_snapshot denotes legacy/default decoding. */
struct RetryableRun { std::string id; CacheKey key; std::string source_path; std::string decoder_snapshot; };
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
  /** Insert or replace one index row on the worker; throws on invalid text or unrepresentable size metadata and storage errors. Does not touch the source file. */
  void put_entry(const CollectionEntry& entry);
  /** Remove only the index row; rejects an empty or embedded-NUL path and returns whether a row existed. Never removes a source file. */
  bool remove_entry(const std::string& path);
  /** Return an owned exact-path entry or absence; rejects empty, embedded-NUL, or oversized paths, throws CollectionCancelled on observed stop, and throws on storage errors. The source file is not opened. */
  std::optional<CollectionEntry> entry(const std::string& path, std::stop_token stop = {});
  /** Return up to the requested bounded page of owned entries; throws CollectionCancelled on observed stop and throws on invalid query or storage errors. */
  std::vector<CollectionEntry> entries(const CollectionQuery& query, std::stop_token stop = {});
  /** Insert a uniquely identified running job; throws on invalid identity, invalid run ID, SQLite error, or duplicate ID. */
  /** Begin a run and persist the immutable source/decoder snapshot used for safe retry. */
  void begin(const std::string& id, const CacheKey& key, const std::string& source_path = {},
             const std::string& decoder_snapshot = {});
  /** Copy a completed cached blob into result; return false on miss, throw on storage error. */
  bool find(const CacheKey& key, std::string& result);
  /** Return 1–10000 persisted runs left retryable by cancellation, stale input, or restart. */
  std::vector<RetryableRun> retryable_runs(std::size_t limit = 100, std::stop_token stop = {});
  /** Return one persisted retryable run by ID, including source and decoder metadata; lookup is independent of the bounded list page. */
  std::optional<RetryableRun> retryable_run(const std::string& id, std::stop_token stop = {});
  /** Discard one retryable run record without touching its source file; throws for an empty, NUL-containing, or overlong ID. */
  bool discard_retryable(const std::string& id);
  /** Atomically discard the supplied retryable run IDs; validates every ID before deleting any row and returns the number removed. */
  std::size_t discard_retryable(const std::vector<std::string>& ids);
  /** Mark a matching running operation retryable after a pre-publication worker failure; the transition clears any partial result and is idempotent for recovery callers; returns false when ID/key do not match a running row or it is already terminal, and throws for invalid inputs. */
  bool mark_retryable(const std::string& id, const CacheKey& key);
  /** Remove the active result for an exact identity; returns false when none exists, throws for an invalid key, and leaves source files and historical runs untouched. */
  bool invalidate(const CacheKey& key);
  /** Revalidate and atomically publish. Oversized results fail closed and leave a running row retryable; an empty blob is a valid persisted result and remains distinct from a cache miss. Stale verification and cancellation likewise preserve retryable recovery. Cancellation after commit begins does not override its outcome. A callback must validate the opened source version; stat/hash checks have filesystem races. */
  Publish publish(const std::string& id, const CacheKey& key, const std::string& result,
                  std::stop_token stop, const std::function<bool()>& revalidate);
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
