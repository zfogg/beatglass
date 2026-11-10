// Private SQLite cache primitive; public API and identity policy need integration review.
#include "beatglass/library/detail/sqlite_cache.hpp"
#include <sqlite3.h>
#include <limits>
#include <functional>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <utility>
#include <unordered_set>

namespace beatglass::library_detail {
bool sqlite_length_representable(std::size_t size) noexcept { return size <= static_cast<std::size_t>(std::numeric_limits<int>::max()); }
namespace {
void validate_text(const std::string& value, const char* field, std::size_t maximum) {
  if (value.empty() || value.size() > maximum || value.find('\0') != std::string::npos ||
      !sqlite_length_representable(value.size()))
    throw std::invalid_argument(std::string("invalid collection ") + field);
}
void validate_collection_entry(const CollectionEntry& e) {
  validate_text(e.path, "path", kMaxCollectionPathBytes);
  validate_text(e.name, "name", kMaxCollectionNameBytes);
  validate_text(e.kind, "kind", kMaxCollectionKindBytes);
  validate_text(e.fingerprint, "fingerprint", kMaxCollectionFingerprintBytes);
}
void validate_key(const CacheKey& key) {
  if (key.content.empty() || key.content.size() > kMaxCacheContentIdentityBytes || key.engine.empty() || key.engine.size() > kMaxCacheEngineVersionBytes ||
      key.config.empty() || key.config.size() > kMaxCacheConfigurationBytes || key.mode.empty() || key.mode.size() > kMaxCacheModeBytes ||
      key.content.find('\0') != std::string::npos || key.engine.find('\0') != std::string::npos ||
      key.config.find('\0') != std::string::npos || key.mode.find('\0') != std::string::npos)
    throw std::invalid_argument("invalid cache identity");
}
void validate_snapshot_text(const std::string& value, const char* field, std::size_t maximum) {
  if (value.size() > maximum || value.find('\0') != std::string::npos ||
      !sqlite_length_representable(value.size()))
    throw std::invalid_argument(std::string("invalid retry snapshot ") + field);
}
}
struct SqliteCache::Impl {
public:
  explicit Impl(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) { auto e = std::string(sqlite3_errmsg(db_)); sqlite3_close(db_); db_ = nullptr; throw std::runtime_error(e); }
    try {
      exec("PRAGMA foreign_keys=ON");
      exec("PRAGMA locking_mode=EXCLUSIVE");
      exec("BEGIN EXCLUSIVE");
      exec("COMMIT");
      sqlite3_stmt* version = nullptr;
      check(sqlite3_prepare_v2(db_, "PRAGMA user_version", -1, &version, nullptr));
      int rc = sqlite3_step(version);
      int v = rc == SQLITE_ROW ? sqlite3_column_int(version, 0) : -1;
      sqlite3_finalize(version);
      if (v != 0 && v != 1 && v != kCacheSchemaVersion) throw std::runtime_error("unsupported database schema");
      if (v == 0) {
        exec("BEGIN IMMEDIATE");
        try {
          exec("CREATE TABLE runs(id TEXT PRIMARY KEY, content TEXT NOT NULL, engine TEXT NOT NULL, config TEXT NOT NULL, mode TEXT NOT NULL, source_path TEXT NOT NULL DEFAULT '', decoder_snapshot TEXT NOT NULL DEFAULT '', status TEXT NOT NULL CHECK(status IN ('running','complete','retryable')), result BLOB, CHECK((status='complete' AND result IS NOT NULL) OR (status!='complete' AND result IS NULL)), UNIQUE(id,content,engine,config,mode,status))");
          exec("CREATE TABLE active(content TEXT NOT NULL, engine TEXT NOT NULL, config TEXT NOT NULL, mode TEXT NOT NULL, run_id TEXT NOT NULL, status TEXT NOT NULL DEFAULT 'complete' CHECK(status='complete'), PRIMARY KEY(content,engine,config,mode), FOREIGN KEY(run_id,content,engine,config,mode,status) REFERENCES runs(id,content,engine,config,mode,status))");
          exec("CREATE TABLE collection_entries(path TEXT PRIMARY KEY, name TEXT NOT NULL, kind TEXT NOT NULL, fingerprint TEXT NOT NULL, size_bytes INTEGER NOT NULL, modified_ns INTEGER NOT NULL)");
          exec("PRAGMA user_version=2"); exec("COMMIT");
        } catch (...) { rollback(); throw; }
      }
      if (v == 1) {
        exec("ALTER TABLE runs ADD COLUMN source_path TEXT NOT NULL DEFAULT ''");
        exec("ALTER TABLE runs ADD COLUMN decoder_snapshot TEXT NOT NULL DEFAULT ''");
        exec("PRAGMA user_version=2");
      }
      // Runs left running by a previous process are never cache hits.
      exec("UPDATE runs SET status='retryable' WHERE status='running'");
    } catch (...) { sqlite3_close(db_); db_ = nullptr; throw; }
  }
  ~Impl() { sqlite3_close(db_); }
  // All calls, including destruction, belong to one database worker.
  void begin(const std::string& id, const CacheKey& key, const std::string& source_path, const std::string& decoder_snapshot) {
    validate_key(key);
    validate_snapshot_text(source_path, "source path", kMaxCollectionPathBytes);
    validate_snapshot_text(decoder_snapshot, "decoder snapshot", kMaxCacheConfigurationBytes);
    if (id.empty() || id.size() > kMaxCacheRunIdBytes || id.find('\0') != std::string::npos) throw std::invalid_argument("invalid run ID");
    Statement s(db_, "INSERT INTO runs(id,content,engine,config,mode,status,source_path,decoder_snapshot) VALUES(?,?,?,?,?,'running',?,?) ON CONFLICT(id) DO UPDATE SET status='running',source_path=excluded.source_path,decoder_snapshot=excluded.decoder_snapshot WHERE runs.status='retryable' AND runs.content=excluded.content AND runs.engine=excluded.engine AND runs.config=excluded.config AND runs.mode=excluded.mode");
    bind(s.p, 1, id); bind_key(s.p, 2, key); bind(s.p, 6, source_path); bind(s.p, 7, decoder_snapshot); done(s.p);
  }
  bool find(const CacheKey& key, std::string& result) {
    validate_key(key);
    Statement s(db_, "SELECT r.result FROM active a JOIN runs r ON r.id=a.run_id WHERE a.content=? AND a.engine=? AND a.config=? AND a.mode=?");
    bind_key(s.p, 1, key); int rc = sqlite3_step(s.p);
    if (rc == SQLITE_DONE) return false;
    if (rc != SQLITE_ROW) throw std::runtime_error(sqlite3_errmsg(db_));
    auto p = static_cast<const char*>(sqlite3_column_blob(s.p, 0));
    auto n = sqlite3_column_bytes(s.p, 0); result.assign(p ? p : "", static_cast<std::size_t>(n)); return true;
  }
  std::vector<RetryableRun> retryable_runs(std::size_t limit, std::stop_token stop) {
    if (limit == 0 || limit > kMaxRetryableRuns) throw std::invalid_argument("invalid retryable run limit");
    Statement s(db_, "SELECT id,content,engine,config,mode,source_path,decoder_snapshot FROM runs WHERE status='retryable' ORDER BY id LIMIT ?");
    check(sqlite3_bind_int64(s.p, 1, static_cast<sqlite3_int64>(limit)));
    std::vector<RetryableRun> out; int rc = SQLITE_ROW;
    while ((rc = sqlite3_step(s.p)) == SQLITE_ROW) {
      if (stop.stop_requested()) throw CollectionCancelled();
      out.push_back({reinterpret_cast<const char*>(sqlite3_column_text(s.p, 0)),
                     {reinterpret_cast<const char*>(sqlite3_column_text(s.p, 1)),
                      reinterpret_cast<const char*>(sqlite3_column_text(s.p, 2)),
                      reinterpret_cast<const char*>(sqlite3_column_text(s.p, 3)),
                     reinterpret_cast<const char*>(sqlite3_column_text(s.p, 4))}, reinterpret_cast<const char*>(sqlite3_column_text(s.p, 5)), reinterpret_cast<const char*>(sqlite3_column_text(s.p, 6))});
    }
    if (rc != SQLITE_DONE) throw std::runtime_error(sqlite3_errmsg(db_));
    return out;
  }
  std::optional<RetryableRun> retryable_run(const std::string& id, std::stop_token stop) {
    if (id.empty() || id.size() > kMaxCacheRunIdBytes || id.find('\0') != std::string::npos)
      throw std::invalid_argument("invalid run ID");
    Statement s(db_, "SELECT id,content,engine,config,mode,source_path,decoder_snapshot FROM runs WHERE id=? AND status='retryable'");
    bind(s.p, 1, id);
    if (stop.stop_requested()) throw CollectionCancelled();
    const int rc = sqlite3_step(s.p);
    if (stop.stop_requested()) throw CollectionCancelled();
    if (rc == SQLITE_DONE) return std::nullopt;
    if (rc != SQLITE_ROW) throw std::runtime_error(sqlite3_errmsg(db_));
    return RetryableRun{reinterpret_cast<const char*>(sqlite3_column_text(s.p, 0)),
                        {reinterpret_cast<const char*>(sqlite3_column_text(s.p, 1)),
                         reinterpret_cast<const char*>(sqlite3_column_text(s.p, 2)),
                         reinterpret_cast<const char*>(sqlite3_column_text(s.p, 3)),
                         reinterpret_cast<const char*>(sqlite3_column_text(s.p, 4))},
                        reinterpret_cast<const char*>(sqlite3_column_text(s.p, 5)),
                        reinterpret_cast<const char*>(sqlite3_column_text(s.p, 6))};
  }
  bool discard_retryable(const std::string& id) {
    if (id.empty() || id.size() > kMaxCacheRunIdBytes || id.find('\0') != std::string::npos)
      throw std::invalid_argument("invalid run ID");
    Statement s(db_, "DELETE FROM runs WHERE id=? AND status='retryable'");
    bind(s.p, 1, id);
    done(s.p);
    return sqlite3_changes(db_) != 0;
  }
  std::size_t discard_retryable(const std::vector<std::string>& ids) {
    if (ids.empty() || ids.size() > kMaxRetryableRuns) throw std::invalid_argument("invalid retryable run batch");
    std::unordered_set<std::string> unique;
    for (const auto& id : ids) {
      if (id.empty() || id.size() > kMaxCacheRunIdBytes || id.find('\0') != std::string::npos)
        throw std::invalid_argument("invalid run ID");
      if (!unique.insert(id).second) throw std::invalid_argument("duplicate retryable run ID");
    }
    exec("BEGIN IMMEDIATE");
    std::size_t removed = 0;
    try {
      for (const auto& id : ids) {
        Statement s(db_, "DELETE FROM runs WHERE id=? AND status='retryable'");
        bind(s.p, 1, id); done(s.p); removed += static_cast<std::size_t>(sqlite3_changes(db_));
      }
      exec("COMMIT"); return removed;
    } catch (...) { rollback(); throw; }
  }
  bool mark_retryable(const std::string& id, const CacheKey& key) {
    validate_key(key);
    if (id.empty() || id.size() > 4096 || id.find('\0') != std::string::npos)
      throw std::invalid_argument("invalid run ID");
    Statement s(db_, "UPDATE runs SET status='retryable',result=NULL WHERE id=? AND content=? AND engine=? AND config=? AND mode=? AND status='running'");
    bind(s.p, 1, id); bind_key(s.p, 2, key); done(s.p);
    return sqlite3_changes(db_) == 1;
  }
  bool invalidate(const CacheKey& key) {
    validate_key(key);
    Statement s(db_, "DELETE FROM active WHERE content=? AND engine=? AND config=? AND mode=?");
    bind_key(s.p, 1, key); done(s.p); return sqlite3_changes(db_) == 1;
  }
  void put_entry(const CollectionEntry& e) {
    validate_collection_entry(e);
    if (e.size_bytes > static_cast<std::uint64_t>(std::numeric_limits<sqlite3_int64>::max()))
      throw std::invalid_argument("collection size is not representable");
    Statement s(db_, "INSERT INTO collection_entries(path,name,kind,fingerprint,size_bytes,modified_ns) VALUES(?,?,?,?,?,?) ON CONFLICT(path) DO UPDATE SET name=excluded.name,kind=excluded.kind,fingerprint=excluded.fingerprint,size_bytes=excluded.size_bytes,modified_ns=excluded.modified_ns");
    bind(s.p,1,e.path); bind(s.p,2,e.name); bind(s.p,3,e.kind); bind(s.p,4,e.fingerprint);
    check(sqlite3_bind_int64(s.p,5,static_cast<sqlite3_int64>(e.size_bytes))); check(sqlite3_bind_int64(s.p,6,e.modified_ns)); done(s.p);
  }
  bool remove_entry(const std::string& path) { validate_text(path, "path", kMaxCollectionPathBytes); Statement s(db_, "DELETE FROM collection_entries WHERE path=?"); bind(s.p,1,path); done(s.p); return sqlite3_changes(db_) == 1; }
  std::optional<CollectionEntry> entry(const std::string& path, std::stop_token stop) {
    if (path.empty() || path.size() > kMaxCollectionPathBytes || path.find('\0') != std::string::npos ||
        !sqlite_length_representable(path.size())) throw std::invalid_argument("invalid collection path");
    if (stop.stop_requested()) throw CollectionCancelled();
    Statement s(db_, "SELECT path,name,kind,fingerprint,size_bytes,modified_ns FROM collection_entries WHERE path=?"); bind(s.p,1,path); const int rc=sqlite3_step(s.p); if(rc==SQLITE_DONE)return std::nullopt; if(rc!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db_)); return read_entry(s.p);
  }
  std::vector<CollectionEntry> entries(const CollectionQuery& q, std::stop_token stop) {
    if (q.limit == 0 || q.limit > kMaxCollectionPageSize) throw std::invalid_argument("invalid collection page size");
    if (q.search.size() > kMaxCollectionSearchBytes || q.search.find('\0') != std::string::npos || !sqlite_length_representable(q.search.size()) ||
        q.offset > kMaxCollectionOffset)
      throw std::invalid_argument("invalid collection query");
    if (stop.stop_requested()) throw CollectionCancelled();
    Statement s(db_, "SELECT path,name,kind,fingerprint,size_bytes,modified_ns FROM collection_entries WHERE lower(name) LIKE lower(?) || '%' ORDER BY lower(name),name,path LIMIT ? OFFSET ?"); bind(s.p,1,q.search); check(sqlite3_bind_int64(s.p,2,static_cast<sqlite3_int64>(q.limit))); check(sqlite3_bind_int64(s.p,3,static_cast<sqlite3_int64>(q.offset))); std::vector<CollectionEntry> out; int rc=SQLITE_ROW; while((rc=sqlite3_step(s.p))==SQLITE_ROW){ if(stop.stop_requested()) throw CollectionCancelled(); out.push_back(read_entry(s.p)); } if(rc!=SQLITE_DONE) throw std::runtime_error(sqlite3_errmsg(db_)); return out;
  }
  // The worker rechecks evidence inside the transaction. The supplied callback
  // must compare the opened input's current version with its pre-analysis evidence.
  // A stat/hash check cannot rule out all concurrent filesystem mutations.
  Publish publish(const std::string& id, const CacheKey& key, const std::string& result,
                  std::stop_token stop, const std::function<bool()>& revalidate) {
    try {
      exec("BEGIN IMMEDIATE");
      // A cancelled or stale run remains retryable.  Do this inside the same
      // transaction so a caller can immediately begin a replacement run while
      // preserving the previous active result.
      auto retryable = [&] {
        Statement update(db_, "UPDATE runs SET status='retryable',result=NULL WHERE id=? AND content=? AND engine=? AND config=? AND mode=? AND status='running'");
        bind(update.p, 1, id); bind_key(update.p, 2, key); done(update.p);
        return sqlite3_changes(db_) == 1;
      };
      // Empty is a valid completed payload: cache presence is represented by
      // the completed run/active identity, not by result-string truthiness.
      if (result.size() > kMaxCachedResultBytes || !sqlite_length_representable(result.size())) {
        if (!retryable()) { rollback(); return Publish::error; }
        exec("COMMIT"); return Publish::error;
      }
      if (stop.stop_requested()) {
        // Cancellation is idempotent for a run already made retryable by a
        // prior stale/cancelled publication; do not turn that terminal state
        // into an unexpected storage error.
        if (!retryable()) { rollback(); return Publish::cancelled; }
        exec("COMMIT"); return Publish::cancelled;
      }
      if (!revalidate()) {
        if (!retryable()) { rollback(); return Publish::error; }
        exec("COMMIT"); return Publish::stale_input;
      }
      Statement update(db_, "UPDATE runs SET status='complete',result=? WHERE id=? AND content=? AND engine=? AND config=? AND mode=? AND status IN ('running','retryable')");
      check(sqlite3_bind_blob(update.p, 1, result.data(), static_cast<int>(result.size()), SQLITE_TRANSIENT));
      bind(update.p, 2, id); bind_key(update.p, 3, key); done(update.p);
      if (sqlite3_changes(db_) != 1) throw std::runtime_error("unknown or non-running run");
      Statement active(db_, "INSERT INTO active(content,engine,config,mode,run_id) VALUES(?,?,?,?,?) ON CONFLICT(content,engine,config,mode) DO UPDATE SET run_id=excluded.run_id");
      bind_key(active.p, 1, key); bind(active.p, 5, id); done(active.p);
      if (stop.stop_requested()) { rollback(); return Publish::cancelled; }
      // After this point cancellation cannot change the result of COMMIT.
      exec("COMMIT"); return Publish::committed;
    } catch (...) {
      rollback();
      // Preserve recovery after verifier/serialization or transactional
      // failures. A running row must never look like an active cache hit.
      try {
        Statement recover(db_, "UPDATE runs SET status='retryable',result=NULL WHERE id=? AND content=? AND engine=? AND config=? AND mode=? AND status='running'");
        bind(recover.p, 1, id); bind_key(recover.p, 2, key); done(recover.p);
      } catch (...) {
        // Keep the original publication error; recovery can be retried after
        // reopening the worker-owned store.
      }
      return Publish::error;
    }
  }
private:
  CollectionEntry read_entry(sqlite3_stmt* s) { return {reinterpret_cast<const char*>(sqlite3_column_text(s,0)),reinterpret_cast<const char*>(sqlite3_column_text(s,1)),reinterpret_cast<const char*>(sqlite3_column_text(s,2)),reinterpret_cast<const char*>(sqlite3_column_text(s,3)),static_cast<std::uint64_t>(sqlite3_column_int64(s,4)),sqlite3_column_int64(s,5)}; }
  struct Statement {
    sqlite3_stmt* p = nullptr;
    Statement(sqlite3* db, const char* sql) { if (sqlite3_prepare_v2(db, sql, -1, &p, nullptr) != SQLITE_OK) throw std::runtime_error(sqlite3_errmsg(db)); }
    ~Statement() { sqlite3_finalize(p); }
  };
  void check(int rc) { if (rc != SQLITE_OK) throw std::runtime_error(sqlite3_errmsg(db_)); }
  void exec(const char* sql) { check(sqlite3_exec(db_, sql, nullptr, nullptr, nullptr)); }
  void rollback() noexcept { sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr); }
  void bind(sqlite3_stmt* s, int i, const std::string& value) { if (!sqlite_length_representable(value.size())) throw std::length_error("SQLite value too large"); check(sqlite3_bind_text(s, i, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT)); }
  void bind_key(sqlite3_stmt* s, int i, const CacheKey& k) { bind(s,i,k.content); bind(s,i+1,k.engine); bind(s,i+2,k.config); bind(s,i+3,k.mode); }
  void done(sqlite3_stmt* s) { if (sqlite3_step(s) != SQLITE_DONE) throw std::runtime_error(sqlite3_errmsg(db_)); }
  sqlite3* db_ = nullptr;
};
SqliteCache::SqliteCache(const std::string& path) : impl_(std::make_unique<Impl>(path)) {}
SqliteCache::~SqliteCache() = default;
void SqliteCache::begin(const std::string& id, const CacheKey& key, const std::string& source_path, const std::string& decoder_snapshot) { impl_->begin(id, key, source_path, decoder_snapshot); }
bool SqliteCache::find(const CacheKey& key, std::string& result) { return impl_->find(key, result); }
std::vector<RetryableRun> SqliteCache::retryable_runs(std::size_t limit, std::stop_token stop) { return impl_->retryable_runs(limit, stop); }
std::optional<RetryableRun> SqliteCache::retryable_run(const std::string& id, std::stop_token stop) { return impl_->retryable_run(id, stop); }
bool SqliteCache::discard_retryable(const std::string& id) { return impl_->discard_retryable(id); }
std::size_t SqliteCache::discard_retryable(const std::vector<std::string>& ids) { return impl_->discard_retryable(ids); }
bool SqliteCache::mark_retryable(const std::string& id, const CacheKey& key) { return impl_->mark_retryable(id, key); }
bool SqliteCache::invalidate(const CacheKey& key) { return impl_->invalidate(key); }
void SqliteCache::put_entry(const CollectionEntry& e) { impl_->put_entry(e); }
bool SqliteCache::remove_entry(const std::string& path) { return impl_->remove_entry(path); }
std::optional<CollectionEntry> SqliteCache::entry(const std::string& path, std::stop_token stop) { return impl_->entry(path, stop); }
std::vector<CollectionEntry> SqliteCache::entries(const CollectionQuery& query, std::stop_token stop) { return impl_->entries(query, stop); }
Publish SqliteCache::publish(const std::string& id, const CacheKey& key, const std::string& result,
                             std::stop_token stop, const std::function<bool()>& revalidate) {
  return impl_->publish(id, key, result, stop, revalidate);
}
}
