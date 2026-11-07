#pragma once
#include <cstddef>
#include <functional>
#include <memory>
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
/** Provisional worker-owned SQLite cache. Not thread-safe; construct, use, and destroy on one worker. A file-backed instance holds an exclusive SQLite lock for its lifetime, so other connections fail rather than misclassifying active runs as interrupted. No input files are modified. */
class SqliteCache {
public:
  /** Open or migrate a database and recover unfinished runs; throws on errors or unsupported schema. */
  explicit SqliteCache(const std::string& path);
  /** Close the connection. */ ~SqliteCache();
  SqliteCache(const SqliteCache&) = delete;
  SqliteCache& operator=(const SqliteCache&) = delete;
  /** Insert a uniquely identified running job; throws on SQLite error or duplicate ID. */
  void begin(const std::string& id, const CacheKey& key);
  /** Copy a completed cached blob into result; return false on miss, throw on storage error. */
  bool find(const CacheKey& key, std::string& result);
  /** Recover a run outcome after reopening the database. The ID and all key fields must match. Throws on SQLite error. Reopening converts interrupted running rows to retryable; a completed row remains evidence of commit even if superseded. */
  RunRecovery recover(const std::string& id, const CacheKey& key);
  /** Revalidate and atomically publish. Cancellation before commit rolls back; cancellation after commit begins does not override its outcome. A callback must validate the opened source version; stat/hash checks have filesystem races. */
  Publish publish(const std::string& id, const CacheKey& key, const std::string& result,
                  std::stop_token stop, const std::function<bool()>& revalidate);
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
