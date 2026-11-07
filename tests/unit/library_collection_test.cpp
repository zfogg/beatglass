#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/detail/sqlite_cache.hpp"
#include <sqlite3.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
using namespace beatglass::library_detail;
namespace {
struct TempDir {
  std::filesystem::path path = std::filesystem::temp_directory_path() / ("beatglass-collection-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  TempDir() { std::filesystem::create_directory(path); }
  ~TempDir() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
};
void sql(sqlite3* db, const char* statement) { REQUIRE(sqlite3_exec(db, statement, nullptr, nullptr, nullptr) == SQLITE_OK); }
}
TEST_CASE("collection indexing returns bounded owned pages and never deletes audio") {
  TempDir temp; auto audio = temp.path / "track.wav"; { std::ofstream f(audio); f << "user audio"; }
  SqliteCache cache((temp.path / "db.sqlite").string());
  CollectionEntry a; a.path = audio.string(); a.title = "100% tune"; a.artist = "Artist B"; a.content_hash = "sha256:a"; a.size_bytes = 10; a.frames = 44100; a.sample_rate = 44100;
  CollectionEntry b; b.path = "b-path"; b.title = "Another"; b.artist = "Artist A"; b.availability = EntryAvailability::missing;
  CollectionEntry c; c.path = "c-path"; c.title = "Another"; c.artist = "Artist C"; c.availability = EntryAvailability::error; c.diagnostic = "unreadable";
  cache.put_entry(a); cache.put_entry(b); cache.put_entry(c);
  auto detail = cache.entry(a.path); REQUIRE(detail); REQUIRE(detail->frames == 44100); REQUIRE(detail->sample_rate == 44100); REQUIRE(detail->content_hash == "sha256:a");
  auto page = cache.entries({"", CollectionOrder::title, false, 1, 0}); REQUIRE(page.size() == 1); REQUIRE(page[0].path == a.path);
  auto next = cache.entries({"another", CollectionOrder::title, false, 1, 0}); REQUIRE(next.size() == 1); REQUIRE(next[0].path == "b-path");
  auto third = cache.entries({"another", CollectionOrder::title, false, 1, 1}); REQUIRE(third.size() == 1); REQUIRE(third[0].path == "c-path");
  auto literal = cache.entries({"%", CollectionOrder::path, false, 20, 0}); REQUIRE(literal.size() == 1); REQUIRE(literal[0].path == a.path);
  auto reverse = cache.entries({"artist", CollectionOrder::artist, true, 20, 0}); REQUIRE(reverse.size() == 3); REQUIRE(reverse[0].artist == "Artist C");
  a.title = "Updated"; cache.put_entry(a); REQUIRE(cache.entry(a.path)->title == "Updated"); REQUIRE(detail->title == "100% tune");
  std::stop_source cancelled; cancelled.request_stop();
  REQUIRE_THROWS_AS(cache.entries({"", CollectionOrder::path, false, 20, 0}, cancelled.get_token()), CollectionCancelled);
  REQUIRE_THROWS_AS(cache.entry(a.path, cancelled.get_token()), CollectionCancelled);
  REQUIRE(cache.entry(a.path));
  REQUIRE_THROWS(cache.entries({"", CollectionOrder::path, false, 0, 0}));
  REQUIRE_THROWS(cache.entries({std::string(4097, 'x'), CollectionOrder::path, false, 1, 0}));
  auto oversized = a; oversized.title = std::string(16385, 'x'); REQUIRE_THROWS(cache.put_entry(oversized));
  auto nul = a; nul.path = std::string("a\0b", 3); REQUIRE_THROWS(cache.put_entry(nul));
  REQUIRE_THROWS(cache.entries({std::string("a\0b", 3), CollectionOrder::path, false, 1, 0}));
  a.size_bytes = std::numeric_limits<std::uint64_t>::max(); REQUIRE_THROWS(cache.put_entry(a));
  REQUIRE(cache.remove_entry(a.path)); REQUIRE_FALSE(cache.remove_entry(a.path)); REQUIRE_FALSE(cache.entry(a.path));
  REQUIRE(std::filesystem::exists(audio)); std::ifstream f(audio); std::string bytes; std::getline(f, bytes); REQUIRE(bytes == "user audio");
}
TEST_CASE("version one migration preserves existing active cache and rolls back on conflict") {
  TempDir temp; auto path = temp.path / "db.sqlite"; sqlite3* raw = nullptr;
  REQUIRE(sqlite3_open(path.string().c_str(), &raw) == SQLITE_OK);
  sql(raw, "CREATE TABLE runs(id TEXT PRIMARY KEY,content TEXT NOT NULL,engine TEXT NOT NULL,config TEXT NOT NULL,mode TEXT NOT NULL,status TEXT NOT NULL,result BLOB,UNIQUE(id,content,engine,config,mode,status)); CREATE TABLE active(content TEXT NOT NULL,engine TEXT NOT NULL,config TEXT NOT NULL,mode TEXT NOT NULL,run_id TEXT NOT NULL,status TEXT NOT NULL DEFAULT 'complete',PRIMARY KEY(content,engine,config,mode),FOREIGN KEY(run_id,content,engine,config,mode,status) REFERENCES runs(id,content,engine,config,mode,status)); INSERT INTO runs VALUES('old','hash','engine','config','offline','complete',x'6f6c64'); INSERT INTO active(content,engine,config,mode,run_id) VALUES('hash','engine','config','offline','old'); PRAGMA user_version=1;");
  REQUIRE(sqlite3_close(raw) == SQLITE_OK);
  { SqliteCache cache(path.string()); std::string result; REQUIRE(cache.find({"hash","engine","config","offline"}, result)); REQUIRE(result == "old"); CollectionEntry e; e.path="file"; cache.put_entry(e); }
  { SqliteCache cache(path.string()); REQUIRE(cache.entry("file")); std::string result; REQUIRE(cache.find({"hash","engine","config","offline"}, result)); REQUIRE(result == "old"); }
  auto conflict = temp.path / "conflict.sqlite"; REQUIRE(sqlite3_open(conflict.string().c_str(), &raw) == SQLITE_OK);
  sql(raw, "CREATE TABLE collection(sentinel TEXT); INSERT INTO collection VALUES('keep'); PRAGMA user_version=1;"); REQUIRE(sqlite3_close(raw) == SQLITE_OK);
  REQUIRE_THROWS(SqliteCache(conflict.string()));
  REQUIRE(sqlite3_open(conflict.string().c_str(), &raw) == SQLITE_OK); sqlite3_stmt* stmt = nullptr;
  REQUIRE(sqlite3_prepare_v2(raw, "SELECT sentinel FROM collection", -1, &stmt, nullptr) == SQLITE_OK); REQUIRE(sqlite3_step(stmt) == SQLITE_ROW); REQUIRE(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))) == "keep"); sqlite3_finalize(stmt);
  REQUIRE(sqlite3_prepare_v2(raw, "PRAGMA user_version", -1, &stmt, nullptr) == SQLITE_OK); REQUIRE(sqlite3_step(stmt) == SQLITE_ROW); REQUIRE(sqlite3_column_int(stmt, 0) == 1); sqlite3_finalize(stmt); REQUIRE(sqlite3_close(raw) == SQLITE_OK);
}
