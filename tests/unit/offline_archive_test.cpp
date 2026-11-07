#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/detail/offline_archive.hpp"
#include <chrono>
#include <filesystem>
#ifdef __linux__
#include <sqlite3.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
using namespace beatglass;
using namespace beatglass::library_detail;
namespace {
struct TempDb { std::filesystem::path path = std::filesystem::temp_directory_path() / ("beatglass-offline-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".sqlite"); ~TempDb() { std::error_code e; std::filesystem::remove(path,e); } };
struct Verify : FileVersionVerifier { FileVerificationOutcome outcome{FileVerificationOutcome::match}; FileVerificationResult verify(std::stop_token) override { return {outcome, {}}; } };
OfflineAnalysis value() {
  OfflineAnalysis v; v.epoch=5; v.sample_rate=48000; v.observed_through=1000; v.analyzed_through=900; v.engine_version="engine"; v.configuration_id="config"; v.algorithms={{"beat","1"},{"tonal","2"}};
  v.beats={{100,200,{}},{300,400,0.5f}};
  v.tonal_windows={{0,500,0,OfflineEstimate{"C", {}}, OfflineEstimate{"Am", 0.2f}},{100,700,100,OfflineEstimate{"G", 0.3f}, {}}}; return v;
}
OfflineKey key() { return {"sha256:content", "engine", "config", OfflineMode::causal, {{"beat","1"},{"tonal","2"}}}; }
}
TEST_CASE("offline archive preserves optionals and overlapping windows in bounded run pages") {
  TempDb db; FileVersion source; source.content_hash="sha256:content"; source.path="song"; Verify verify;
  { SqliteCache cache(db.path.string()); CollectionEntry entry; entry.path="song"; cache.put_entry(entry); OfflineArchive archive(cache);
    REQUIRE(archive.publish(source,key(),value(),"run-1","song",verify,{}).outcome == PublishOutcome::committed);
    auto p=archive.page(source,key(),"run-1",0,1,0,1,20,verify,{}); REQUIRE(p.status == OfflineReadStatus::hit); REQUIRE(p.page); REQUIRE(p.page->header.epoch==20); REQUIRE(p.page->header.beats.capacity()==0); REQUIRE(p.page->header.tonal_windows.capacity()==0); REQUIRE(p.page->total_beats==2); REQUIRE(p.page->next_beat==1);
    REQUIRE_FALSE(p.page->beats[0].strength); REQUIRE_FALSE(p.page->windows[0].rolling_key->strength); REQUIRE(p.page->windows[0].rolling_key_from == 0);
    auto q=archive.page(source,key(),"run-1",1,1,1,1,20,verify,{}); REQUIRE(q.status == OfflineReadStatus::hit); REQUIRE(q.page); REQUIRE(q.page->beats[0].strength == 0.5f); REQUIRE(q.page->windows[0].start_frame==100); REQUIRE_FALSE(q.page->windows[0].local_chord);
    REQUIRE(archive.page(source,key(),"run-1",0,201,0,1,20,verify,{}).status == OfflineReadStatus::error);
    REQUIRE(archive.publish(source,key(),value(),"run-2","song",verify,{}).outcome == PublishOutcome::committed);
    REQUIRE(archive.page(source,key(),"run-1",0,1,0,1,20,verify,{}).status == OfflineReadStatus::changed);
    auto current=archive.page(source,key(),"",0,1,0,1,20,verify,{}); REQUIRE(current.status == OfflineReadStatus::hit); REQUIRE(current.page->run_id == "run-2");
    verify.outcome=FileVerificationOutcome::changed; REQUIRE(archive.page(source,key(),"run-2",0,1,0,1,20,verify,{}).status == OfflineReadStatus::changed);
  }
  { SqliteCache cache(db.path.string()); OfflineArchive archive(cache); verify.outcome=FileVerificationOutcome::match; REQUIRE(archive.page(source,key(),"run-2",0,2,0,2,21,verify,{}).status == OfflineReadStatus::hit); }
}
TEST_CASE("offline archive rolls back failed association and rejects invalid values") {
  TempDb db; SqliteCache cache(db.path.string()); CollectionEntry row; row.path="song"; row.content_hash="previous"; cache.put_entry(row); OfflineArchive archive(cache); FileVersion source; source.content_hash="sha256:content"; source.path="song"; Verify verify;
  REQUIRE(archive.publish(source,key(),value(),"old","song",verify,{}).outcome==PublishOutcome::committed);
  auto old_hash=cache.entry("song")->content_hash;
  REQUIRE(archive.publish(source,key(),value(),"failed-replacement","missing-row",verify,{}).outcome==PublishOutcome::error);
  REQUIRE(cache.entry("song")->content_hash==old_hash);
  REQUIRE(archive.page(source,key(),"old",0,1,0,1,8,verify,{}).status==OfflineReadStatus::hit);
  auto invalid=value(); invalid.epoch=0; REQUIRE(archive.publish(source,key(),invalid,"invalid","song",verify,{}).outcome==PublishOutcome::error);
  REQUIRE(archive.publish(source,key(),value(),"missing","missing-row",verify,{}).outcome==PublishOutcome::error);
  REQUIRE(cache.entry("song")->content_hash==old_hash); REQUIRE(archive.page(source,key(),"missing",0,1,0,1,8,verify,{}).status == OfflineReadStatus::changed);
  verify.outcome=FileVerificationOutcome::changed; REQUIRE(archive.publish(source,key(),value(),"stale","song",verify,{}).outcome==PublishOutcome::stale_input); REQUIRE(cache.entry("song")->content_hash==old_hash);
  std::stop_source stop; stop.request_stop(); REQUIRE(archive.publish(source,key(),value(),"cancel","song",verify,stop.get_token()).outcome==PublishOutcome::cancelled);
}

TEST_CASE("indexed publication recovers a lost commit acknowledgement after reopen") {
  TempDb db; CacheKey key{"hash","engine","config","mode"};
  { SqliteCache cache(db.path.string()); CollectionEntry entry; entry.path="song"; entry.content_hash="old"; cache.put_entry(entry); cache.begin("run", key);
    REQUIRE(cache.publish_indexed("run",key,"value","song",{},[]{return true;},[]{throw std::runtime_error("lost acknowledgement");}) == Publish::error);
  }
  { SqliteCache cache(db.path.string()); REQUIRE(cache.recover("run",key)==RunRecovery::committed); REQUIRE(cache.entry("song")->content_hash=="hash"); std::string value; REQUIRE(cache.find(key,value)); REQUIRE(value=="value"); }
}

#ifdef __linux__
TEST_CASE("process exit before commit rolls back indexed cache changes") {
  TempDb db; CacheKey key{"hash","engine","config","mode"};
  { SqliteCache cache(db.path.string()); CollectionEntry entry; entry.path="song"; entry.content_hash="old"; cache.put_entry(entry); cache.begin("old", key); REQUIRE(cache.publish("old",key,"old-value",{},[]{return true;})==Publish::committed); }
  pid_t child = fork(); REQUIRE(child >= 0);
  if (child == 0) {
    sqlite3* raw = nullptr; if (sqlite3_open(db.path.string().c_str(), &raw) != SQLITE_OK) _exit(2);
    if (sqlite3_exec(raw, "BEGIN IMMEDIATE; UPDATE collection SET content_hash='new' WHERE path='song'; UPDATE runs SET result='new-value' WHERE id='old';", nullptr, nullptr, nullptr) != SQLITE_OK) _exit(3);
    _exit(0);
  }
  int status=0; REQUIRE(waitpid(child,&status,0)==child); REQUIRE(WIFEXITED(status)); REQUIRE(WEXITSTATUS(status)==0);
  { SqliteCache cache(db.path.string()); REQUIRE(cache.entry("song")->content_hash=="old"); std::string result; REQUIRE(cache.find(key,result)); REQUIRE(result=="old-value"); }
}
#endif
TEST_CASE("indexed publication rolls back a replacement when its row is missing") {
  TempDb db; SqliteCache cache(db.path.string()); CacheKey key{"hash","engine","config","mode"};
  CollectionEntry row; row.path="song"; row.content_hash="old-hash"; cache.put_entry(row);
  cache.begin("old", key); REQUIRE(cache.publish("old",key,"old-value",{},[]{return true;})==Publish::committed);
  cache.begin("new", key); REQUIRE(cache.publish_indexed("new",key,"new-value","missing",{},[]{return true;})==Publish::error);
  std::string active; REQUIRE(cache.find(key,active)); REQUIRE(active=="old-value"); REQUIRE(cache.entry("song")->content_hash=="old-hash");
}
