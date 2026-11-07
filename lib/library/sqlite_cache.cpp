// Private SQLite cache primitive; public API and identity policy need integration review.
#include <sqlite3.h>
#include <functional>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <utility>

namespace beatglass::library_detail {
struct CacheKey { std::string content, engine, config, mode; };
enum class Publish { committed, cancelled, stale_input, error };
class SqliteCache {
public:
  explicit SqliteCache(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) { auto e = std::string(sqlite3_errmsg(db_)); sqlite3_close(db_); db_ = nullptr; throw std::runtime_error(e); }
    try {
      exec("PRAGMA foreign_keys=ON");
      sqlite3_stmt* version = nullptr;
      check(sqlite3_prepare_v2(db_, "PRAGMA user_version", -1, &version, nullptr));
      int rc = sqlite3_step(version);
      int v = rc == SQLITE_ROW ? sqlite3_column_int(version, 0) : -1;
      sqlite3_finalize(version);
      if (v != 0 && v != 1) throw std::runtime_error("unsupported database schema");
      if (v == 0) {
        exec("BEGIN IMMEDIATE");
        try {
          exec("CREATE TABLE runs(id TEXT PRIMARY KEY, content TEXT NOT NULL, engine TEXT NOT NULL, config TEXT NOT NULL, mode TEXT NOT NULL, status TEXT NOT NULL CHECK(status IN ('running','complete','retryable')), result BLOB, CHECK((status='complete' AND result IS NOT NULL) OR (status!='complete' AND result IS NULL)), UNIQUE(id,content,engine,config,mode,status))");
          exec("CREATE TABLE active(content TEXT NOT NULL, engine TEXT NOT NULL, config TEXT NOT NULL, mode TEXT NOT NULL, run_id TEXT NOT NULL, status TEXT NOT NULL DEFAULT 'complete' CHECK(status='complete'), PRIMARY KEY(content,engine,config,mode), FOREIGN KEY(run_id,content,engine,config,mode,status) REFERENCES runs(id,content,engine,config,mode,status))");
          exec("PRAGMA user_version=1"); exec("COMMIT");
        } catch (...) { rollback(); throw; }
      }
      // Runs left running by a previous process are never cache hits.
      exec("UPDATE runs SET status='retryable' WHERE status='running'");
    } catch (...) { sqlite3_close(db_); db_ = nullptr; throw; }
  }
  ~SqliteCache() { sqlite3_close(db_); }
  SqliteCache(const SqliteCache&) = delete;
  SqliteCache& operator=(const SqliteCache&) = delete;
  // All calls, including destruction, belong to one database worker.
  void begin(const std::string& id, const CacheKey& key) {
    Statement s(db_, "INSERT INTO runs(id,content,engine,config,mode,status) VALUES(?,?,?,?,?,'running')");
    bind(s.p, 1, id); bind_key(s.p, 2, key); done(s.p);
  }
  bool find(const CacheKey& key, std::string& result) {
    Statement s(db_, "SELECT r.result FROM active a JOIN runs r ON r.id=a.run_id WHERE a.content=? AND a.engine=? AND a.config=? AND a.mode=?");
    bind_key(s.p, 1, key); int rc = sqlite3_step(s.p);
    if (rc == SQLITE_DONE) return false;
    if (rc != SQLITE_ROW) throw std::runtime_error(sqlite3_errmsg(db_));
    auto p = static_cast<const char*>(sqlite3_column_blob(s.p, 0));
    auto n = sqlite3_column_bytes(s.p, 0); result.assign(p ? p : "", static_cast<std::size_t>(n)); return true;
  }
  // The worker rechecks evidence inside the transaction. The supplied callback
  // must compare the opened input's current version with its pre-analysis evidence.
  // A stat/hash check cannot rule out all concurrent filesystem mutations.
  Publish publish(const std::string& id, const CacheKey& key, const std::string& result,
                  std::stop_token stop, const std::function<bool()>& revalidate) {
    try {
      exec("BEGIN IMMEDIATE");
      if (stop.stop_requested()) { rollback(); return Publish::cancelled; }
      if (!revalidate()) { rollback(); return Publish::stale_input; }
      Statement update(db_, "UPDATE runs SET status='complete',result=? WHERE id=? AND content=? AND engine=? AND config=? AND mode=? AND status='running'");
      check(sqlite3_bind_blob(update.p, 1, result.data(), static_cast<int>(result.size()), SQLITE_TRANSIENT));
      bind(update.p, 2, id); bind_key(update.p, 3, key); done(update.p);
      if (sqlite3_changes(db_) != 1) throw std::runtime_error("unknown or non-running run");
      Statement active(db_, "INSERT INTO active(content,engine,config,mode,run_id) VALUES(?,?,?,?,?) ON CONFLICT(content,engine,config,mode) DO UPDATE SET run_id=excluded.run_id");
      bind_key(active.p, 1, key); bind(active.p, 5, id); done(active.p);
      if (stop.stop_requested()) { rollback(); return Publish::cancelled; }
      // After this point cancellation cannot change the result of COMMIT.
      exec("COMMIT"); return Publish::committed;
    } catch (...) { rollback(); return Publish::error; }
  }
private:
  struct Statement {
    sqlite3_stmt* p = nullptr;
    Statement(sqlite3* db, const char* sql) { if (sqlite3_prepare_v2(db, sql, -1, &p, nullptr) != SQLITE_OK) throw std::runtime_error(sqlite3_errmsg(db)); }
    ~Statement() { sqlite3_finalize(p); }
  };
  void check(int rc) { if (rc != SQLITE_OK) throw std::runtime_error(sqlite3_errmsg(db_)); }
  void exec(const char* sql) { check(sqlite3_exec(db_, sql, nullptr, nullptr, nullptr)); }
  void rollback() noexcept { sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr); }
  void bind(sqlite3_stmt* s, int i, const std::string& value) { check(sqlite3_bind_text(s, i, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT)); }
  void bind_key(sqlite3_stmt* s, int i, const CacheKey& k) { bind(s,i,k.content); bind(s,i+1,k.engine); bind(s,i+2,k.config); bind(s,i+3,k.mode); }
  void done(sqlite3_stmt* s) { if (sqlite3_step(s) != SQLITE_DONE) throw std::runtime_error(sqlite3_errmsg(db_)); }
  sqlite3* db_ = nullptr;
};
}
