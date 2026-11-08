// Private SQLite cache primitive; public API and identity policy need integration review.
#include "beatglass/library/detail/sqlite_cache.hpp"
#include <sqlite3.h>
#include <limits>
#include <functional>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <utility>

namespace beatglass::library_detail {
bool sqlite_length_representable(std::size_t size) noexcept { return size <= static_cast<std::size_t>(std::numeric_limits<int>::max()); }
namespace {
void validate_key(const CacheKey& key) {
  if (key.content.empty() || key.content.size() > 512 || key.engine.empty() || key.engine.size() > 4096 ||
      key.config.empty() || key.config.size() > 4096 || key.mode.empty() || key.mode.size() > 128 ||
      key.content.find('\0') != std::string::npos || key.engine.find('\0') != std::string::npos ||
      key.config.find('\0') != std::string::npos || key.mode.find('\0') != std::string::npos)
    throw std::invalid_argument("invalid cache identity");
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
      if (v != 0 && v != 1 && v != 2) throw std::runtime_error("unsupported database schema");
      if (v == 0) {
        exec("BEGIN IMMEDIATE");
        try {
          exec("CREATE TABLE runs(id TEXT PRIMARY KEY, content TEXT NOT NULL, engine TEXT NOT NULL, config TEXT NOT NULL, mode TEXT NOT NULL, status TEXT NOT NULL CHECK(status IN ('running','complete','retryable')), result BLOB, CHECK((status='complete' AND result IS NOT NULL) OR (status!='complete' AND result IS NULL)), UNIQUE(id,content,engine,config,mode,status))");
          exec("CREATE TABLE active(content TEXT NOT NULL, engine TEXT NOT NULL, config TEXT NOT NULL, mode TEXT NOT NULL, run_id TEXT NOT NULL, status TEXT NOT NULL DEFAULT 'complete' CHECK(status='complete'), PRIMARY KEY(content,engine,config,mode), FOREIGN KEY(run_id,content,engine,config,mode,status) REFERENCES runs(id,content,engine,config,mode,status))");
          exec("PRAGMA user_version=1"); exec("COMMIT");
        } catch (...) { rollback(); throw; }
        v = 1;
      }
      if (v == 1) {
        exec("BEGIN IMMEDIATE");
        try {
          exec("CREATE TABLE collection(path TEXT PRIMARY KEY NOT NULL CHECK(length(path)>0), title TEXT NOT NULL, artist TEXT NOT NULL, content_hash TEXT NOT NULL, size_bytes INTEGER NOT NULL CHECK(size_bytes>=0), modified_ns INTEGER NOT NULL, frames INTEGER CHECK(frames>=0), sample_rate INTEGER CHECK(sample_rate>0), availability INTEGER NOT NULL CHECK(availability BETWEEN 0 AND 2), diagnostic TEXT NOT NULL)");
          exec("PRAGMA user_version=2"); exec("COMMIT");
        } catch (...) { rollback(); throw; }
      }
      // Runs left running by a previous process are never cache hits.
      exec("UPDATE runs SET status='retryable' WHERE status='running'");
    } catch (...) { sqlite3_close(db_); db_ = nullptr; throw; }
  }
  ~Impl() { sqlite3_close(db_); }
  // All calls, including destruction, belong to one database worker.
  void put_entry(const CollectionEntry& e) {
    if (e.path.empty() || e.path.find('\0') != std::string::npos || e.title.find('\0') != std::string::npos || e.artist.find('\0') != std::string::npos || e.content_hash.find('\0') != std::string::npos || e.diagnostic.find('\0') != std::string::npos || e.path.size() > 262144 || e.title.size() > 16384 || e.artist.size() > 16384 || e.diagnostic.size() > 16384 || e.content_hash.size() > 512 || e.size_bytes > static_cast<std::uint64_t>(std::numeric_limits<sqlite3_int64>::max()) ||
        (e.frames && *e.frames > static_cast<std::uint64_t>(std::numeric_limits<sqlite3_int64>::max())) ||
        (e.sample_rate && *e.sample_rate == 0) || static_cast<int>(e.availability) < 0 || static_cast<int>(e.availability) > 2)
      throw std::invalid_argument("invalid collection entry");
    Statement s(db_, "INSERT INTO collection(path,title,artist,content_hash,size_bytes,modified_ns,frames,sample_rate,availability,diagnostic) VALUES(?,?,?,?,?,?,?,?,?,?) ON CONFLICT(path) DO UPDATE SET title=excluded.title,artist=excluded.artist,content_hash=excluded.content_hash,size_bytes=excluded.size_bytes,modified_ns=excluded.modified_ns,frames=excluded.frames,sample_rate=excluded.sample_rate,availability=excluded.availability,diagnostic=excluded.diagnostic");
    bind(s.p,1,e.path); bind(s.p,2,e.title); bind(s.p,3,e.artist); bind(s.p,4,e.content_hash);
    check(sqlite3_bind_int64(s.p,5,static_cast<sqlite3_int64>(e.size_bytes))); check(sqlite3_bind_int64(s.p,6,e.modified_ns));
    if (e.frames) check(sqlite3_bind_int64(s.p,7,static_cast<sqlite3_int64>(*e.frames))); else check(sqlite3_bind_null(s.p,7));
    if (e.sample_rate) check(sqlite3_bind_int64(s.p,8,*e.sample_rate)); else check(sqlite3_bind_null(s.p,8));
    check(sqlite3_bind_int(s.p,9,static_cast<int>(e.availability))); bind(s.p,10,e.diagnostic); done(s.p);
  }
  bool remove_entry(const std::string& path) {
    Statement s(db_, "DELETE FROM collection WHERE path=?"); bind(s.p,1,path); done(s.p); return sqlite3_changes(db_) == 1;
  }
  std::optional<CollectionEntry> entry(const std::string& path, std::stop_token stop) {
    if (stop.stop_requested()) throw CollectionCancelled(); Progress progress(db_, stop);
    Statement s(db_, "SELECT path,title,artist,content_hash,size_bytes,modified_ns,frames,sample_rate,availability,diagnostic FROM collection WHERE path=?");
    bind(s.p,1,path); int rc = sqlite3_step(s.p); if (stop.stop_requested()) throw CollectionCancelled(); if (rc == SQLITE_DONE) return std::nullopt;
    if (rc != SQLITE_ROW) throw std::runtime_error(sqlite3_errmsg(db_)); return read_entry(s.p);
  }
  std::vector<CollectionEntry> entries(const CollectionQuery& q, std::stop_token stop) {
    if (stop.stop_requested()) throw CollectionCancelled(); Progress progress(db_, stop);
    if (q.limit == 0 || q.limit > 200 || q.search.size() > 4096 || q.search.find('\0') != std::string::npos) throw std::invalid_argument("invalid collection page size");
    const char* order = nullptr;
    switch (q.order) { case CollectionOrder::path: order = "path"; break; case CollectionOrder::title: order = "title"; break; case CollectionOrder::artist: order = "artist"; break; default: throw std::invalid_argument("invalid collection order"); }
    std::string sql = "SELECT path,title,artist,content_hash,size_bytes,modified_ns,frames,sample_rate,availability,diagnostic FROM collection WHERE instr(lower(path),lower(?1))>0 OR instr(lower(title),lower(?1))>0 OR instr(lower(artist),lower(?1))>0 ORDER BY ";
    sql += order; sql += " COLLATE NOCASE "; sql += q.descending ? "DESC" : "ASC"; sql += ", path COLLATE BINARY ASC LIMIT ?2 OFFSET ?3";
    Statement s(db_, sql.c_str()); bind(s.p,1,q.search); check(sqlite3_bind_int64(s.p,2,q.limit)); check(sqlite3_bind_int64(s.p,3,q.offset));
    std::vector<CollectionEntry> result;
    for (;;) { int rc = sqlite3_step(s.p); if (stop.stop_requested()) throw CollectionCancelled(); if (rc == SQLITE_DONE) break; if (rc != SQLITE_ROW) throw std::runtime_error(sqlite3_errmsg(db_)); result.push_back(read_entry(s.p)); }
    return result;
  }
  void begin(const std::string& id, const CacheKey& key) {
    validate_key(key);
    if (id.empty() || id.size() > 4096 || id.find('\0') != std::string::npos) throw std::invalid_argument("invalid run ID");
    Statement s(db_, "INSERT INTO runs(id,content,engine,config,mode,status) VALUES(?,?,?,?,?,'running')");
    bind(s.p, 1, id); bind_key(s.p, 2, key); done(s.p);
  }
  bool find(const CacheKey& key, std::string& result) {
    Statement s(db_, "SELECT length(r.result), CASE WHEN length(r.result)<=8388608 THEN r.result ELSE NULL END FROM active a JOIN runs r ON r.id=a.run_id WHERE a.content=? AND a.engine=? AND a.config=? AND a.mode=?");
    bind_key(s.p, 1, key); int rc = sqlite3_step(s.p);
    if (rc == SQLITE_DONE) return false;
    if (rc != SQLITE_ROW) throw std::runtime_error(sqlite3_errmsg(db_));
    const auto length = sqlite3_column_int64(s.p, 0);
    if (length < 0 || length > 8 * 1024 * 1024) throw std::runtime_error("offline result too large");
    auto p = static_cast<const char*>(sqlite3_column_blob(s.p, 1));
    auto n = sqlite3_column_bytes(s.p, 1); result.assign(p ? p : "", static_cast<std::size_t>(n)); return true;
  }
  std::optional<std::string> active_run(const CacheKey& key) {
    Statement s(db_, "SELECT run_id FROM active WHERE content=? AND engine=? AND config=? AND mode=?"); bind_key(s.p, 1, key);
    int rc = sqlite3_step(s.p); if (rc == SQLITE_DONE) return {}; if (rc != SQLITE_ROW) throw std::runtime_error(sqlite3_errmsg(db_));
    auto text = reinterpret_cast<const char*>(sqlite3_column_text(s.p, 0)); if (!text) throw std::runtime_error("invalid run ID"); return std::string(text);
  }
  bool find_run(const std::string& id, const CacheKey& key, std::string& result) {
    Statement s(db_, "SELECT length(result), CASE WHEN length(result)<=8388608 THEN result ELSE NULL END FROM runs WHERE id=? AND content=? AND engine=? AND config=? AND mode=? AND status='complete'");
    bind(s.p, 1, id); bind_key(s.p, 2, key); int rc = sqlite3_step(s.p);
    if (rc == SQLITE_DONE) return false; if (rc != SQLITE_ROW) throw std::runtime_error(sqlite3_errmsg(db_));
    auto length = sqlite3_column_int64(s.p, 0); if (length < 0 || length > 8 * 1024 * 1024) throw std::runtime_error("offline result too large");
    auto p = static_cast<const char*>(sqlite3_column_blob(s.p, 1)); auto n = sqlite3_column_bytes(s.p, 1);
    result.assign(p ? p : "", static_cast<std::size_t>(n)); return true;
  }
  RunRecovery recover(const std::string& id, const CacheKey& key) {
    Statement s(db_, "SELECT status FROM runs WHERE id=? AND content=? AND engine=? AND config=? AND mode=?");
    bind(s.p, 1, id); bind_key(s.p, 2, key);
    const int rc = sqlite3_step(s.p);
    if (rc == SQLITE_DONE) return RunRecovery::missing;
    if (rc != SQLITE_ROW) throw std::runtime_error(sqlite3_errmsg(db_));
    const auto status = reinterpret_cast<const char*>(sqlite3_column_text(s.p, 0));
    if (!status) throw std::runtime_error("invalid run status");
    if (std::string(status) == "complete") return RunRecovery::committed;
    if (std::string(status) == "retryable") return RunRecovery::retryable;
    throw std::runtime_error("cannot recover a currently running run; reopen first");
  }
  bool committed_run(const std::string& id) {
    Statement s(db_, "SELECT status FROM runs WHERE id=?"); bind(s.p, 1, id);
    int rc = sqlite3_step(s.p); if (rc == SQLITE_DONE) return false;
    if (rc != SQLITE_ROW) throw std::runtime_error(sqlite3_errmsg(db_));
    auto status = reinterpret_cast<const char*>(sqlite3_column_text(s.p, 0));
    if (!status) throw std::runtime_error("invalid run status");
    return std::string(status) == "complete";
  }
  // The worker rechecks evidence inside the transaction. The supplied callback
  // must compare the opened input's current version with its pre-analysis evidence.
  // A stat/hash check cannot rule out all concurrent filesystem mutations.
  Publish publish(const std::string& id, const CacheKey& key, const std::string& result,
                  std::stop_token stop, const std::function<bool()>& revalidate,
                  const std::function<void()>& after_commit_probe, const std::string* indexed_path = nullptr) {
    try {
      if (!sqlite_length_representable(result.size()) || result.size() > 8 * 1024 * 1024) return Publish::error;
      exec("BEGIN IMMEDIATE");
      if (stop.stop_requested()) { rollback(); return Publish::cancelled; }
      if (!revalidate()) { rollback(); return Publish::stale_input; }
      Statement update(db_, "UPDATE runs SET status='complete',result=? WHERE id=? AND content=? AND engine=? AND config=? AND mode=? AND status='running'");
      check(sqlite3_bind_blob(update.p, 1, result.data(), static_cast<int>(result.size()), SQLITE_TRANSIENT));
      bind(update.p, 2, id); bind_key(update.p, 3, key); done(update.p);
      if (sqlite3_changes(db_) != 1) throw std::runtime_error("unknown or non-running run");
      Statement active(db_, "INSERT INTO active(content,engine,config,mode,run_id) VALUES(?,?,?,?,?) ON CONFLICT(content,engine,config,mode) DO UPDATE SET run_id=excluded.run_id");
      bind_key(active.p, 1, key); bind(active.p, 5, id); done(active.p);
      if (indexed_path) { Statement row(db_, "UPDATE collection SET content_hash=? WHERE path=?"); bind(row.p, 1, key.content); bind(row.p, 2, *indexed_path); done(row.p); if (sqlite3_changes(db_) != 1) throw std::runtime_error("indexed entry missing"); }
      if (stop.stop_requested()) { rollback(); return Publish::cancelled; }
      // After this point cancellation cannot change the result of COMMIT.
      exec("COMMIT"); if (after_commit_probe) after_commit_probe(); return Publish::committed;
    } catch (...) { rollback(); return Publish::error; }
  }
  Publish associate(const CacheKey& key, const std::string& path, std::stop_token stop, const std::function<bool()>& revalidate) {
    try {
      exec("BEGIN IMMEDIATE");
      if (stop.stop_requested()) { rollback(); return Publish::cancelled; }
      if (!revalidate()) { rollback(); return Publish::stale_input; }
      Statement exists(db_, "SELECT 1 FROM active WHERE content=? AND engine=? AND config=? AND mode=?"); bind_key(exists.p, 1, key);
      if (sqlite3_step(exists.p) != SQLITE_ROW) throw std::runtime_error("active run missing");
      Statement row(db_, "UPDATE collection SET content_hash=? WHERE path=?"); bind(row.p, 1, key.content); bind(row.p, 2, path); done(row.p);
      if (sqlite3_changes(db_) != 1) throw std::runtime_error("indexed entry missing");
      if (stop.stop_requested()) { rollback(); return Publish::cancelled; }
      exec("COMMIT"); return Publish::committed;
    } catch (...) { rollback(); return Publish::error; }
  }
private:
  struct Progress {
    sqlite3* db; const std::stop_token* token;
    Progress(sqlite3* connection, const std::stop_token& stop) : db(connection), token(&stop) {
      sqlite3_progress_handler(db, 1000, [](void* value) { return static_cast<Progress*>(value)->token->stop_requested() ? 1 : 0; }, this);
    }
    ~Progress() { sqlite3_progress_handler(db, 0, nullptr, nullptr); }
  };
  struct Statement {
    sqlite3_stmt* p = nullptr;
    Statement(sqlite3* db, const char* sql) { if (sqlite3_prepare_v2(db, sql, -1, &p, nullptr) != SQLITE_OK) throw std::runtime_error(sqlite3_errmsg(db)); }
    ~Statement() { sqlite3_finalize(p); }
  };
  CollectionEntry read_entry(sqlite3_stmt* s) {
    auto str = [this,s](int col) { auto p = sqlite3_column_text(s,col); auto n = sqlite3_column_bytes(s,col); if (n > 262144) throw std::runtime_error("collection text too large"); if (!p) throw std::runtime_error("invalid collection text"); return std::string(reinterpret_cast<const char*>(p), static_cast<std::size_t>(n)); };
    CollectionEntry e; e.path=str(0); e.title=str(1); e.artist=str(2); e.content_hash=str(3);
    auto size = sqlite3_column_int64(s,4); auto frames = sqlite3_column_int64(s,6); auto rate = sqlite3_column_int64(s,7); auto availability = sqlite3_column_int(s,8);
    if (size < 0 || (sqlite3_column_type(s,6) != SQLITE_NULL && frames < 0) || (sqlite3_column_type(s,7) != SQLITE_NULL && (rate <= 0 || rate > std::numeric_limits<std::uint32_t>::max())) || availability < 0 || availability > 2) throw std::runtime_error("invalid collection values");
    e.size_bytes=static_cast<std::uint64_t>(size); e.modified_ns=sqlite3_column_int64(s,5);
    if (sqlite3_column_type(s,6) != SQLITE_NULL) e.frames=static_cast<std::uint64_t>(frames);
    if (sqlite3_column_type(s,7) != SQLITE_NULL) e.sample_rate=static_cast<std::uint32_t>(rate);
    e.availability=static_cast<EntryAvailability>(availability); e.diagnostic=str(9); return e;
  }
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
void SqliteCache::put_entry(const CollectionEntry& e) { impl_->put_entry(e); }
bool SqliteCache::remove_entry(const std::string& path) { return impl_->remove_entry(path); }
std::optional<CollectionEntry> SqliteCache::entry(const std::string& path, std::stop_token stop) { return impl_->entry(path, stop); }
std::vector<CollectionEntry> SqliteCache::entries(const CollectionQuery& q, std::stop_token stop) { return impl_->entries(q, stop); }
void SqliteCache::begin(const std::string& id, const CacheKey& key) { impl_->begin(id, key); }
bool SqliteCache::find(const CacheKey& key, std::string& result) { return impl_->find(key, result); }
std::optional<std::string> SqliteCache::active_run(const CacheKey& key) { return impl_->active_run(key); }
bool SqliteCache::find_run(const std::string& id, const CacheKey& key, std::string& result) { return impl_->find_run(id, key, result); }
RunRecovery SqliteCache::recover(const std::string& id, const CacheKey& key) { return impl_->recover(id, key); }
bool SqliteCache::committed_run(const std::string& id) { return impl_->committed_run(id); }
Publish SqliteCache::publish(const std::string& id, const CacheKey& key, const std::string& result,
                             std::stop_token stop, const std::function<bool()>& revalidate,
                  const std::function<void()>& after_commit_probe) {
  return impl_->publish(id, key, result, stop, revalidate, after_commit_probe);
}
Publish SqliteCache::publish_indexed(const std::string& id, const CacheKey& key, const std::string& result, const std::string& path,
                                    std::stop_token stop, const std::function<bool()>& revalidate, const std::function<void()>& after_commit_probe) {
  if (path.empty()) return Publish::error;
  return impl_->publish(id, key, result, stop, revalidate, after_commit_probe, &path);
}
Publish SqliteCache::associate(const CacheKey& key, const std::string& path, std::stop_token stop, const std::function<bool()>& revalidate) {
  if (path.empty()) return Publish::error;
  return impl_->associate(key, path, stop, revalidate);
}

}
