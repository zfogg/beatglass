#include <catch2/catch_test_macros.hpp>
#include "library_bridge.hpp"
#include "beatglass/library/detail/file_snapshot.hpp"
#include <chrono>
#include <condition_variable>
#include <deque>
#include <fstream>
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

TEST_CASE("indexed cache operations fail closed without source exclusion") {
  auto dir = std::filesystem::temp_directory_path() / ("beatglass-indexed-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directory(dir); auto db = dir / "cache.sqlite"; auto audio = dir / "tone.aiff";
  std::filesystem::copy_file(std::filesystem::path(BEATGLASS_FIXTURES) / "tone.aiff", audio);
  auto key = audio.generic_string();
  { TempoAnalysisStore store(db.string()); CollectionEntry row; row.path = key; row.title = "Indexed"; store.put_entry(row);
    auto seed = analyze_snapshot_to_store(store, audio, "indexed-v1", "default", 10);
    REQUIRE(seed.status == FileJobStatus::committed);
    auto direct = analyze_indexed_snapshot_to_store(store, key, audio, "indexed-v1", "default", 11);
    REQUIRE(direct.status == FileJobStatus::error); REQUIRE_FALSE(direct.value);
    REQUIRE(store.entry(key)->content_hash.empty()); }
  Dispatcher queue; std::optional<LibraryBridge::Result> result;
  {
    LibraryBridge bridge(db.string(), [&](auto call) { queue.post(std::move(call)); });
    bridge.analyze_indexed(key, audio, "indexed-v1", "default", 11, [&](auto r) { result = std::move(r); });
    auto call = queue.take(); REQUIRE(call); call(); REQUIRE(result->error.find("disabled") != std::string::npos);
    bridge.lookup_indexed(key, audio, "indexed-v1", "default", 12, [&](auto r) { result = std::move(r); });
    call = queue.take(); REQUIRE(call); call(); REQUIRE(result->error.find("disabled") != std::string::npos);
  }
  { TempoAnalysisStore store(db.string()); REQUIRE(store.entry(key)->content_hash.empty()); }
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
    LibraryBridge bridge(db.string(),[&](auto call){queue.post(std::move(call));});
    bridge.analyze_composite(audio,81,[&](auto value){result=std::move(value);});
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
