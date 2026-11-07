#include <catch2/catch_test_macros.hpp>
#include "library_bridge.hpp"
#include "beatglass/library/detail/file_snapshot.hpp"
#include <chrono>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <limits>
#include <sqlite3.h>
#include <mutex>
#include <thread>
using namespace beatglass::library_detail;
namespace {
struct Dispatcher {
  std::mutex mutex; std::condition_variable ready; std::deque<std::function<void()>> calls;
  void post(std::function<void()> call) { std::lock_guard lock(mutex); calls.push_back(std::move(call)); ready.notify_one(); }
  std::function<void()> take() { std::unique_lock lock(mutex); if (!ready.wait_for(lock, std::chrono::seconds(10), [&]{ return !calls.empty(); })) return {}; auto call = std::move(calls.front()); calls.pop_front(); return call; }
};
}
TEST_CASE("library bridge delivers owned results on dispatcher and suppresses stale work") {
  auto dir = std::filesystem::temp_directory_path() / ("beatglass-bridge-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directory(dir); auto db = dir / "cache.sqlite";
  { TempoAnalysisStore store(db.string()); CollectionEntry row; row.path = "track"; row.title = "Title"; store.put_entry(row); }
  Dispatcher queue; int delivered = 0; std::optional<LibraryBridge::Result> result; auto gui = std::this_thread::get_id();
  std::function<void()> after_destroy;
  {
    LibraryBridge bridge(db.string(), [&](auto call){ queue.post(std::move(call)); });
    bridge.entries({}, [&](auto value){ REQUIRE(std::this_thread::get_id() == gui); ++delivered; result = std::move(value); });
    auto first = queue.take(); REQUIRE(first);
    bridge.detail("track", [&](auto value){ REQUIRE(std::this_thread::get_id() == gui); ++delivered; result = std::move(value); });
    first(); REQUIRE(delivered == 0);
    auto second = queue.take(); REQUIRE(second); second(); REQUIRE(delivered == 1);
    REQUIRE(result); auto detail = std::get<LibraryBridge::Detail>(result->value); REQUIRE(detail); REQUIRE(detail->title == "Title");
    bridge.analyze("unused", "engine", "config", 0, [&](auto value){ result = std::move(value); ++delivered; });
    auto third = queue.take(); REQUIRE(third); third(); REQUIRE(delivered == 2); REQUIRE_FALSE(result->error.empty());
    auto audio = dir / "tone.aiff";
    std::filesystem::copy_file(std::filesystem::path(BEATGLASS_FIXTURES) / "tone.aiff", audio);
    bridge.analyze(audio, "bridge-test", "default", 42, [&](auto value){ result = std::move(value); ++delivered; });
    auto real = queue.take(); REQUIRE(real); real(); REQUIRE(delivered == 3);
    auto analysis = std::get<LibraryBridge::Analysis>(result->value); INFO(analysis.diagnostic);
    REQUIRE(analysis.status == FileJobStatus::committed); REQUIRE(analysis.value); REQUIRE(analysis.value->result.observed_through.epoch == 42);
    auto identity = analysis.value->identity;
    auto ticket = bridge.lookup(audio, identity, 43, [&](auto value){ result = std::move(value); ++delivered; });
    auto lookup_call = queue.take(); REQUIRE(lookup_call); REQUIRE(ticket->result()); lookup_call();
    auto lookup = std::get<LibraryBridge::Lookup>(result->value); REQUIRE(lookup.status == LibraryBridge::LookupStatus::hit);
    REQUIRE(lookup.value); REQUIRE(lookup.value->result.observed_through.epoch == 43); REQUIRE(delivered == 4);
    auto other = identity; other.configuration_id = "missing";
    bridge.lookup(audio, other, 44, [&](auto value){ result = std::move(value); ++delivered; });
    auto miss_call = queue.take(); REQUIRE(miss_call); miss_call(); REQUIRE(std::get<LibraryBridge::Lookup>(result->value).status == LibraryBridge::LookupStatus::miss);
    { std::ofstream out(audio, std::ios::binary | std::ios::app); out << 'x'; }
    bridge.lookup(audio, identity, 45, [&](auto value){ result = std::move(value); ++delivered; });
    auto changed_call = queue.take(); REQUIRE(changed_call); changed_call(); REQUIRE(std::get<LibraryBridge::Lookup>(result->value).status == LibraryBridge::LookupStatus::changed);
    REQUIRE(ticket->result()); REQUIRE(std::get<LibraryBridge::Lookup>(ticket->result()->value).status == LibraryBridge::LookupStatus::hit);
    bridge.entries({}, [&](auto){ ++delivered; }); auto fourth = queue.take(); REQUIRE(fourth); bridge.cancel(); fourth(); REQUIRE(delivered == 6);
    bridge.entries({}, [&](auto){ ++delivered; }); after_destroy = queue.take(); REQUIRE(after_destroy);
  }
  after_destroy(); REQUIRE(delivered == 6);
  std::error_code ignored; std::filesystem::remove_all(dir, ignored);
}

TEST_CASE("library bridge retains cancellation of pending requests") {
  auto dir = std::filesystem::temp_directory_path() / ("beatglass-cancel-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directory(dir);
  std::mutex mutex; std::condition_variable ready; bool entered = false, release = false;
  {
    LibraryBridge bridge((dir / "cache.sqlite").string(), [&](auto) {
      std::unique_lock lock(mutex); entered = true; ready.notify_one(); ready.wait(lock, [&] { return release; });
    });
    bridge.entries({}, {});
    { std::unique_lock lock(mutex); REQUIRE(ready.wait_for(lock, std::chrono::seconds(10), [&] { return entered; })); }
    auto pending = bridge.detail("track", {});
    bridge.cancel();
    auto result = pending->result(); REQUIRE(result); REQUIRE(result->cancelled);
    { std::lock_guard lock(mutex); release = true; } ready.notify_one();
  }
  std::error_code ignored; std::filesystem::remove_all(dir, ignored);
}

TEST_CASE("indexed analysis links content for verified lookup and rejects changed source") {
  auto dir = std::filesystem::temp_directory_path() / ("beatglass-indexed-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directory(dir); auto db = dir / "cache.sqlite"; auto audio = dir / "tone.aiff";
  std::filesystem::copy_file(std::filesystem::path(BEATGLASS_FIXTURES) / "tone.aiff", audio);
  auto key = audio.generic_string();
  { TempoAnalysisStore store(db.string()); CollectionEntry row; row.path = key; row.title = "Indexed"; store.put_entry(row); }
  auto blocked_audio = dir / "blocked.aiff";
  std::filesystem::copy_file(audio, blocked_audio); auto blocked_key = blocked_audio.generic_string();
  { TempoAnalysisStore store(db.string()); CollectionEntry row; row.path = blocked_key; row.title = "Blocked"; store.put_entry(row); }
  sqlite3* raw = nullptr; REQUIRE(sqlite3_open(db.string().c_str(), &raw) == SQLITE_OK);
  REQUIRE(sqlite3_exec(raw, "CREATE TRIGGER deny_index_update BEFORE UPDATE ON collection WHEN NEW.title='Blocked' BEGIN SELECT RAISE(ABORT, 'blocked'); END", nullptr, nullptr, nullptr) == SQLITE_OK);
  REQUIRE(sqlite3_close(raw) == SQLITE_OK);
  Dispatcher queue; std::optional<LibraryBridge::Result> result;
  {
    LibraryBridge bridge(db.string(), [&](auto call) { queue.post(std::move(call)); });
    bridge.analyze_indexed(blocked_key, blocked_audio, "blocked-v1", "default", 8, [&](auto r) { result = std::move(r); });
    auto blocked_call = queue.take(); REQUIRE(blocked_call); blocked_call();
    REQUIRE(std::get<LibraryBridge::Analysis>(result->value).status == FileJobStatus::error);
    bridge.lookup_indexed(blocked_key, blocked_audio, "blocked-v1", "default", 9, [&](auto r) { result = std::move(r); });
    blocked_call = queue.take(); REQUIRE(blocked_call); blocked_call(); REQUIRE(std::get<LibraryBridge::Lookup>(result->value).status == LibraryBridge::LookupStatus::miss);
    bridge.analyze(blocked_audio, "blocked-v1", "default", 14, [&](auto r) { result = std::move(r); });
    blocked_call = queue.take(); REQUIRE(blocked_call); blocked_call(); REQUIRE(std::get<LibraryBridge::Analysis>(result->value).status == FileJobStatus::committed);
    bridge.lookup_indexed(key, audio, "indexed-v1", "default", 10, [&](auto r) { result = std::move(r); });
    auto call = queue.take(); REQUIRE(call); call(); REQUIRE(std::get<LibraryBridge::Lookup>(result->value).status == LibraryBridge::LookupStatus::miss);
    bridge.analyze_indexed(key, audio, "indexed-v1", "default", 11, [&](auto r) { result = std::move(r); });
    call = queue.take(); REQUIRE(call); call(); REQUIRE(result->association_error.empty());
    REQUIRE(std::get<LibraryBridge::Analysis>(result->value).status == FileJobStatus::committed);
    bridge.lookup_indexed(key, audio, "indexed-v1", "default", 12, [&](auto r) { result = std::move(r); });
    call = queue.take(); REQUIRE(call); call(); auto found = std::get<LibraryBridge::Lookup>(result->value);
    REQUIRE(found.status == LibraryBridge::LookupStatus::hit); REQUIRE(found.value); REQUIRE(found.value->result.observed_through.epoch == 12);
    { std::ofstream out(audio, std::ios::binary | std::ios::app); out << 'x'; }
    bridge.lookup_indexed(key, audio, "indexed-v1", "default", 13, [&](auto r) { result = std::move(r); });
    call = queue.take(); REQUIRE(call); call(); REQUIRE(std::get<LibraryBridge::Lookup>(result->value).status == LibraryBridge::LookupStatus::changed);
  }
  std::error_code ignored; std::filesystem::remove_all(dir, ignored);
}

TEST_CASE("browsing does not cancel queued analysis") {
  auto dir = std::filesystem::temp_directory_path() / ("beatglass-lanes-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directory(dir); auto audio = dir / "tone.aiff";
  std::filesystem::copy_file(std::filesystem::path(BEATGLASS_FIXTURES) / "tone.aiff", audio);
  std::mutex mutex; std::condition_variable ready; bool entered = false, release = false; Dispatcher queue;
  {
    LibraryBridge bridge((dir / "cache.sqlite").string(), [&](auto call) {
      { std::unique_lock lock(mutex); if (!entered) { entered = true; ready.notify_one(); ready.wait(lock, [&] { return release; }); } }
      queue.post(std::move(call));
    });
    bridge.entries({}, {});
    { std::unique_lock lock(mutex); REQUIRE(ready.wait_for(lock, std::chrono::seconds(10), [&] { return entered; })); }
    auto analysis = bridge.analyze(audio, "lanes-v1", "default", 77, {});
    auto browse = bridge.detail("missing", {});
    { std::lock_guard lock(mutex); release = true; } ready.notify_one();
    for (int i = 0; i < 3; ++i) { auto call = queue.take(); REQUIRE(call); call(); }
    REQUIRE(analysis->result()); auto value = std::get<LibraryBridge::Analysis>(analysis->result()->value);
    REQUIRE(value.status == FileJobStatus::committed); REQUIRE(browse->result()); REQUIRE_FALSE(browse->result()->cancelled);
  }
  std::error_code ignored; std::filesystem::remove_all(dir, ignored);
}

TEST_CASE("offline bridge delivers an owned verified page and rejects changed source") {
  auto dir = std::filesystem::temp_directory_path() / ("beatglass-offline-ui-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directory(dir); auto db = dir / "cache.sqlite"; auto audio = dir / "tone.aiff";
  std::filesystem::copy_file(std::filesystem::path(BEATGLASS_FIXTURES) / "tone.aiff", audio); auto key = audio.generic_string();
  auto snapshot = SnapshotOpen::create(audio); REQUIRE(snapshot.status == SnapshotStatus::ok);
  beatglass::FileVersion file{audio, snapshot.value->size(), *snapshot.value->modified_unix_ns(), snapshot.value->digest()};
  OfflineKey identity{file.content_hash,"engine","config",beatglass::OfflineMode::causal,{{"tempo","1"}}};
  beatglass::OfflineAnalysis data; data.epoch=1; data.sample_rate=48000; data.observed_through=512; data.analyzed_through=512; data.engine_version="engine"; data.configuration_id="config"; data.algorithms=identity.algorithms; data.beats={{100,200,{}}};
  struct SourceVerifier : beatglass::FileVersionVerifier { FileSnapshot& snapshot; explicit SourceVerifier(FileSnapshot& s) : snapshot(s) {} beatglass::FileVerificationResult verify(std::stop_token stop) override { return {snapshot.verify(stop)==SnapshotStatus::ok ? beatglass::FileVerificationOutcome::match : beatglass::FileVerificationOutcome::changed,{}}; } } verifier(*snapshot.value);
  { TempoAnalysisStore store(db.string()); CollectionEntry entry; entry.path=key; store.put_entry(entry); OfflineArchive archive(store.offline_cache()); REQUIRE(archive.publish(file,identity,data,"offline-run",key,verifier,{}).outcome==beatglass::PublishOutcome::committed); }
  snapshot.value.reset(); Dispatcher queue; std::optional<LibraryBridge::Result> result;
  { LibraryBridge bridge(db.string(), [&](auto call) { queue.post(std::move(call)); });
    bridge.offline_page(key,audio,identity,"",0,1,0,1,9,[&](auto r){ result=std::move(r); });
    auto call=queue.take(); REQUIRE(call); call(); auto page=std::get<OfflineReadResult>(result->value); REQUIRE(page.status==OfflineReadStatus::hit); REQUIRE(page.page); REQUIRE(page.page->run_id=="offline-run"); REQUIRE(page.page->header.epoch==9); REQUIRE_FALSE(page.page->beats[0].strength);
    { std::ofstream out(audio,std::ios::binary|std::ios::app); out << 'x'; }
    bridge.offline_page(key,audio,identity,"offline-run",0,1,0,1,10,[&](auto r){ result=std::move(r); });
    call=queue.take(); REQUIRE(call); call(); REQUIRE(std::get<OfflineReadResult>(result->value).status==OfflineReadStatus::changed);
  }
  std::error_code ignored; std::filesystem::remove_all(dir,ignored);
}

#ifdef BEATGLASS_PRIVATE_COMPOSITE_BRIDGE
TEST_CASE("composite bridge returns real unverified evidence without publishing cache") {
  auto dir = std::filesystem::temp_directory_path() / ("beatglass-composite-bridge-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directory(dir); auto db=dir/"cache.sqlite"; auto audio=dir/"tone.aiff";
  std::filesystem::copy_file(std::filesystem::path(BEATGLASS_FIXTURES)/"tone.aiff",audio);
  Dispatcher queue; std::optional<LibraryBridge::Result> result;
  {
    LibraryBridge bridge(db.string(),[&](auto call){queue.post(std::move(call));},std::make_shared<beatglass::SourceEpochAllocator>(80));
    bridge.start_composite(audio,[&](auto value){result=std::move(value);});
    auto call=queue.take(); REQUIRE(call); call(); REQUIRE(result);
    auto composite=std::get<LibraryBridge::Composite>(result->value); INFO(composite.diagnostic);
    REQUIRE(composite.status==CompositeJobStatus::complete); REQUIRE(composite.value); REQUIRE_FALSE(composite.value->cache_reusable());
    REQUIRE(composite.value->value.epoch==81); REQUIRE(composite.value->value.engine_version.empty()); REQUIRE(composite.value->value.configuration_id.empty());
  }
  auto snapshot=SnapshotOpen::create(audio); REQUIRE(snapshot.status==SnapshotStatus::ok);
  beatglass::FileVersion source{audio,snapshot.value->size(),*snapshot.value->modified_unix_ns(),snapshot.value->digest()};
  struct RejectVerifier : beatglass::FileVersionVerifier { int calls{}; beatglass::FileVerificationResult verify(std::stop_token) override { ++calls; return {beatglass::FileVerificationOutcome::match,{}}; } } verifier;
  { TempoAnalysisStore store(db.string()); CollectionEntry entry; entry.path=audio.generic_string(); store.put_entry(entry); OfflineArchive archive(store.offline_cache());
    OfflineKey fabricated{source.content_hash,"unverified","unverified",beatglass::OfflineMode::causal,{{"unverified","unverified"}}};
    const auto& evidence=std::get<LibraryBridge::Composite>(result->value).value->value;
    REQUIRE(archive.publish(source,fabricated,evidence,"must-not-publish",entry.path,verifier,{}).outcome==beatglass::PublishOutcome::error); REQUIRE(verifier.calls==0);
  }
  std::error_code ignored; std::filesystem::remove_all(dir,ignored);
}
#endif

TEST_CASE("library job issuance shares the coordinator allocator and fails on exhaustion") {
  auto dir = std::filesystem::temp_directory_path() / ("beatglass-epoch-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directory(dir); auto audio=dir/"tone.aiff";
  std::filesystem::copy_file(std::filesystem::path(BEATGLASS_FIXTURES)/"tone.aiff",audio);
  { TempoAnalysisStore seed((dir/"two.sqlite").string()); CollectionEntry entry; entry.path=audio.generic_string(); seed.put_entry(entry); }
  beatglass::SourceEpochAllocator epochs; Dispatcher first_queue, second_queue; std::optional<LibraryBridge::Result> result;
  {
    LibraryBridge one((dir/"one.sqlite").string(),[&](auto call){first_queue.post(std::move(call));},epochs);
    LibraryBridge two((dir/"two.sqlite").string(),[&](auto call){second_queue.post(std::move(call));},epochs);
    REQUIRE(one.new_session_epoch() == 1);
    auto ticket=two.start_analysis(audio,"epoch-test","default",[&](auto r){result=std::move(r);});
    auto call=second_queue.take(); REQUIRE(call); call(); REQUIRE(ticket->result());
    auto job=std::get<LibraryBridge::Analysis>(result->value); REQUIRE(job.status==FileJobStatus::committed); REQUIRE(job.value); REQUIRE(job.value->result.observed_through.epoch==2);
    REQUIRE(one.new_session_epoch() == 3);
    auto indexed=two.start_indexed_analysis(audio.generic_string(),audio,"epoch-test","default",[&](auto r){result=std::move(r);});
    call=second_queue.take(); REQUIRE(call); call(); REQUIRE(indexed->result()); auto hit=std::get<LibraryBridge::Analysis>(result->value);
    REQUIRE(hit.status==FileJobStatus::cache_hit); REQUIRE(hit.value); REQUIRE(hit.value->result.observed_through.epoch==4);
  }
  {
    auto exhausted=std::make_shared<beatglass::SourceEpochAllocator>(std::numeric_limits<std::uint64_t>::max()); Dispatcher queue;
    LibraryBridge bridge((dir/"exhausted.sqlite").string(),[&](auto call){queue.post(std::move(call));},exhausted);
    auto ticket=bridge.start_analysis(dir/"nonexistent","engine","config",{}); auto call=queue.take(); REQUIRE(call); call(); REQUIRE(ticket->result()); REQUIRE(ticket->result()->error=="analysis epoch must be nonzero");
  }
  std::error_code ignored; std::filesystem::remove_all(dir,ignored);
}
