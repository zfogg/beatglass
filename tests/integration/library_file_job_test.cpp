#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/detail/file_job.hpp"
#include "beatglass/library/detail/job_executor.hpp"
#include "beatglass/library/detail/file_snapshot.hpp"
#include "beatglass/library/detail/file_fingerprint.hpp"
#include "beatglass/library/detail/snapshot_store_job.hpp"
#include "beatglass/library/detail/tempo_analysis_store.hpp"
#ifndef _WIN32
#include <sndfile.h>
#endif
#include <future>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <vector>
using namespace beatglass::library_detail;
namespace {
struct TempDirectory {
  std::filesystem::path path = std::filesystem::temp_directory_path() / ("beatglass-job-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  TempDirectory() { std::filesystem::create_directory(path); }
  ~TempDirectory() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
};
}
TEST_CASE("real tempo analysis is reused after reopen and invalidated by content or configuration") {
  TempDirectory temp; auto input = temp.path / "audio"; auto db = temp.path / "library.sqlite";
  auto fixtures = std::filesystem::path(BEATGLASS_FIXTURES);
  std::filesystem::copy_file(fixtures / "tone.aiff", input);
  int progress = 0;
  {
    SqliteCache cache(db.string());
    auto first = analyze_cached_tempo(cache, input, "aubio-test-v1", "config1", {}, [&](auto) { ++progress; });
    INFO(first.diagnostic); REQUIRE(first.status == FileJobStatus::committed); REQUIRE(first.result); REQUIRE(progress > 0);
  }
  {
    SqliteCache cache(db.string()); progress = 0;
    auto hit = analyze_cached_tempo(cache, input, "aubio-test-v1", "config1", {}, [&](auto) { ++progress; });
    INFO(hit.diagnostic); REQUIRE(hit.status == FileJobStatus::cache_hit); REQUIRE(hit.result); REQUIRE(progress == 0);
    auto changed_config = analyze_cached_tempo(cache, input, "aubio-test-v1", "config2", {}, [&](auto) { ++progress; });
    INFO(changed_config.diagnostic); REQUIRE(changed_config.status == FileJobStatus::committed); REQUIRE(progress > 0);
    { std::ofstream out(input, std::ios::binary | std::ios::app); out << 'z'; }
    progress = 0;
    auto changed_file = analyze_cached_tempo(cache, input, "aubio-test-v1", "config1", {}, [&](auto) { ++progress; });
    INFO(changed_file.diagnostic); REQUIRE(changed_file.status == FileJobStatus::committed); REQUIRE(progress > 0);
  }
}
TEST_CASE("real tempo job cooperatively cancels and does not publish") {
  TempDirectory temp; auto input = temp.path / "audio";
  std::filesystem::copy_file(std::filesystem::path(BEATGLASS_FIXTURES) / "tone.aiff", input);
  SqliteCache cache((temp.path / "library.sqlite").string()); std::stop_source stop;
  auto cancelled = analyze_cached_tempo(cache, input, "aubio-test-v1", "config", stop.get_token(), [&](auto) { stop.request_stop(); });
  REQUIRE(cancelled.status == FileJobStatus::cancelled);
  auto retry = analyze_cached_tempo(cache, input, "aubio-test-v1", "config");
  INFO(retry.diagnostic); REQUIRE(retry.status == FileJobStatus::committed);
}

TEST_CASE("real tempo job rejects a file changed during analysis") {
  TempDirectory temp; auto input = temp.path / "audio";
  std::filesystem::copy_file(std::filesystem::path(BEATGLASS_FIXTURES) / "tone.aiff", input);
  SqliteCache cache((temp.path / "library.sqlite").string()); bool modified = false;
  auto changed = analyze_cached_tempo(cache, input, "aubio-test-v1", "config", {}, [&](auto) {
    if (!modified) { modified = true; std::ofstream out(input, std::ios::binary | std::ios::app); out << 'x'; }
  });
  INFO(changed.diagnostic); REQUIRE(modified); REQUIRE(changed.status == FileJobStatus::stale_input);
  auto retry = analyze_cached_tempo(cache, input, "aubio-test-v1", "config");
  INFO(retry.diagnostic); REQUIRE(retry.status == FileJobStatus::committed);
}

TEST_CASE("tracked queue cancels a real tempo job without publishing") {
  TempDirectory temp; auto input = temp.path / "audio";
  std::filesystem::copy_file(std::filesystem::path(BEATGLASS_FIXTURES) / "tone.aiff", input);
  JobExecutor executor(1); std::promise<void> started, release; auto gate = release.get_future().share();
  auto handle = executor.submit_tracked(0, [&](std::stop_token stop, const auto& progress) {
    SqliteCache cache((temp.path / "library.sqlite").string()); bool signalled = false;
    auto result = analyze_cached_tempo(cache, input, "aubio-test-v1", "config", stop, [&](auto frames) {
      progress(frames); if (!signalled) { signalled = true; started.set_value(); gate.wait(); }
    });
    return result.status == FileJobStatus::cancelled ? TrackedState::cancelled : TrackedState::failed;
  });
  REQUIRE(handle); REQUIRE(started.get_future().wait_for(std::chrono::seconds(5)) == std::future_status::ready);
  REQUIRE(handle->frames() > 0); handle->cancel(); release.set_value(); executor.shutdown();
  REQUIRE(handle->state() == TrackedState::cancelled);
  SqliteCache cache((temp.path / "library.sqlite").string());
  auto retry = analyze_cached_tempo(cache, input, "aubio-test-v1", "config");
  INFO(retry.diagnostic); REQUIRE(retry.status == FileJobStatus::committed);
}

TEST_CASE("decoder reads snapshot while source replacement blocks publication") {
#ifndef _WIN32
  TempDirectory temp; auto input = temp.path / "audio";
  std::filesystem::copy_file(std::filesystem::path(BEATGLASS_FIXTURES) / "tone.aiff", input);
  auto opened = SnapshotOpen::create(input); INFO(opened.diagnostic); REQUIRE(opened.status == SnapshotStatus::ok);
  SF_INFO info{}; auto decoder = sf_open_fd(opened.value->borrowed_descriptor(), SFM_READ, &info, 0);
  REQUIRE(decoder != nullptr); REQUIRE(info.channels > 0);
  std::vector<float> samples(static_cast<std::size_t>(info.channels) * 256);
  REQUIRE(sf_readf_float(decoder, samples.data(), 256) > 0);
  std::filesystem::rename(input, temp.path / "old");
  std::filesystem::copy_file(std::filesystem::path(BEATGLASS_FIXTURES) / "tone.aiff", input);
  REQUIRE(sf_readf_float(decoder, samples.data(), 256) > 0);
  REQUIRE(sf_close(decoder) == 0);
  SqliteCache cache((temp.path / "library.sqlite").string());
  CacheKey key{opened.value->digest(), "engine", "config", "offline"}; cache.begin("old", key);
  REQUIRE(cache.publish("old", key, "old-result", {}, [] { return true; }) == Publish::committed);
  cache.begin("run", key);
  REQUIRE(cache.publish("run", key, "result", {}, [&] { return opened.value->verify() == SnapshotStatus::ok; }) == Publish::stale_input);
  std::string result; REQUIRE(cache.find(key, result)); REQUIRE(result == "old-result");
#endif
}

TEST_CASE("snapshot virtual job reuses decoded bytes and rejects source mutation") {
  TempDirectory temp; auto input = temp.path / "audio"; auto database = temp.path / "library.sqlite";
  auto fixtures = std::filesystem::path(BEATGLASS_FIXTURES);
  std::filesystem::copy_file(fixtures / "tone.aiff", input); int progress = 0;
  {
    SqliteCache cache(database.string());
    auto first = analyze_snapshot_tempo(cache, input, "aubio-snapshot-v1", "config1", {}, [&](auto) { ++progress; });
    INFO(first.diagnostic); REQUIRE(first.status == FileJobStatus::committed); REQUIRE(first.result); REQUIRE(progress > 0);
    auto direct = beatglass::analyze_file(input);
    REQUIRE(first.result->metadata.sample_rate == direct.metadata.sample_rate);
    REQUIRE(first.result->metadata.channels == direct.metadata.channels);
    REQUIRE(first.result->metadata.frames == direct.metadata.frames);
    REQUIRE(first.result->bpm == direct.bpm); REQUIRE(first.result->beats == direct.beats); REQUIRE(first.result->state == direct.state);
  }
  {
    SqliteCache cache(database.string()); progress = 0;
    auto reused = analyze_snapshot_tempo(cache, input, "aubio-snapshot-v1", "config1", {}, [&](auto) { ++progress; });
    INFO(reused.diagnostic); REQUIRE(reused.status == FileJobStatus::cache_hit); REQUIRE(progress == 0);
    auto changed = analyze_snapshot_tempo(cache, input, "aubio-snapshot-v1", "config2", {}, [&](auto) {
      if (progress++ == 0) { std::ofstream out(input, std::ios::binary | std::ios::app); out << 'x'; }
    });
    INFO(changed.diagnostic); REQUIRE(changed.status == FileJobStatus::stale_input); REQUIRE(progress > 0);
    std::filesystem::copy_file(fixtures / "tone.aiff", input, std::filesystem::copy_options::overwrite_existing);
    auto old = analyze_snapshot_tempo(cache, input, "aubio-snapshot-v1", "config1");
    INFO(old.diagnostic); REQUIRE(old.status == FileJobStatus::cache_hit);
    auto config = analyze_snapshot_tempo(cache, input, "aubio-snapshot-v1", "config2");
    INFO(config.diagnostic); REQUIRE(config.status == FileJobStatus::committed);
    { std::ofstream out(input, std::ios::binary | std::ios::app); out << 'z'; }
    auto content = analyze_snapshot_tempo(cache, input, "aubio-snapshot-v1", "config1");
    INFO(content.diagnostic); REQUIRE(content.status == FileJobStatus::committed);
  }
}
TEST_CASE("snapshot virtual job preserves old result when path is replaced") {
  TempDirectory temp; auto input = temp.path / "audio"; auto fixtures = std::filesystem::path(BEATGLASS_FIXTURES);
  std::filesystem::copy_file(fixtures / "tone.aiff", input);
  SqliteCache cache((temp.path / "library.sqlite").string());
  auto first = analyze_snapshot_tempo(cache, input, "snapshot-v1", "old");
  INFO(first.diagnostic); REQUIRE(first.status == FileJobStatus::committed);
  bool replaced = false;
  auto replacement = analyze_snapshot_tempo(cache, input, "snapshot-v1", "new", {}, [&](auto) {
    if (!replaced) { replaced = true; std::filesystem::rename(input, temp.path / "old-file"); std::filesystem::copy_file(fixtures / "tone.aiff", input); }
  });
  INFO(replacement.diagnostic); REQUIRE(replaced); REQUIRE(replacement.status == FileJobStatus::stale_input);
  std::filesystem::remove(input); std::filesystem::rename(temp.path / "old-file", input);
  auto hit = analyze_snapshot_tempo(cache, input, "snapshot-v1", "old");
  INFO(hit.diagnostic); REQUIRE(hit.status == FileJobStatus::cache_hit);
}
TEST_CASE("snapshot virtual job cancels during decoding and retries") {
  TempDirectory temp; auto input = temp.path / "audio";
  std::filesystem::copy_file(std::filesystem::path(BEATGLASS_FIXTURES) / "tone.aiff", input);
  SqliteCache cache((temp.path / "library.sqlite").string()); std::stop_source stop;
  auto cancelled = analyze_snapshot_tempo(cache, input, "snapshot-v1", "config", stop.get_token(), [&](auto) { stop.request_stop(); });
  INFO(cancelled.diagnostic); REQUIRE(cancelled.status == FileJobStatus::cancelled);
  auto retry = analyze_snapshot_tempo(cache, input, "snapshot-v1", "config");
  INFO(retry.diagnostic); REQUIRE(retry.status == FileJobStatus::committed);
}

TEST_CASE("snapshot virtual cache handles generated WAV and available codec fixtures") {
  TempDirectory temp; auto db = temp.path / "library.sqlite"; auto wav = temp.path / "generated.wav";
  {
    std::ofstream out(wav, std::ios::binary);
    auto u16 = [&](unsigned n) { out.put(static_cast<char>(n & 255)); out.put(static_cast<char>((n >> 8) & 255)); };
    auto u32 = [&](unsigned n) { u16(n & 65535); u16(n >> 16); };
    constexpr unsigned frames = 8192;
    out.write("RIFF", 4); u32(36 + frames * 2); out.write("WAVEfmt ", 8); u32(16);
    u16(1); u16(1); u32(44100); u32(88200); u16(2); u16(16); out.write("data", 4); u32(frames * 2);
    for (unsigned i = 0; i < frames; ++i) u16(i % 2 ? 8192 : 0);
    REQUIRE(out.good());
  }
  auto fixtures = std::filesystem::path(BEATGLASS_FIXTURES);
  std::vector<std::filesystem::path> paths{wav, fixtures / "tone.aiff"};
#ifdef BEATGLASS_REQUIRE_V1_CODECS
  paths.push_back(fixtures / "tone.flac"); paths.push_back(fixtures / "tone.mp3");
#endif
  std::uint64_t epoch = 1000;
  for (const auto& path : paths) {
    CAPTURE(path.string()); auto before = fingerprint_file(path); REQUIRE(before.status == FingerprintStatus::ok); int blocks = 0;
    { SqliteCache cache(db.string()); auto first = analyze_snapshot_tempo(cache, path, "codec-test-v1", "default", {}, [&](auto) { ++blocks; });
      INFO(first.diagnostic); REQUIRE(first.status == FileJobStatus::committed); REQUIRE(first.result); REQUIRE(blocks > 0); }
    { SqliteCache cache(db.string()); blocks = 0; auto hit = analyze_snapshot_tempo(cache, path, "codec-test-v1", "default", {}, [&](auto) { ++blocks; });
      INFO(hit.diagnostic); REQUIRE(hit.status == FileJobStatus::cache_hit); REQUIRE(blocks == 0); }
    { TempoAnalysisStore store(db.string()); blocks = 0; auto first = analyze_snapshot_to_store(store, path, "shared-codec-test-v1", "default", epoch++, {}, [&](auto) { ++blocks; });
      INFO(first.diagnostic); REQUIRE(first.status == FileJobStatus::committed); REQUIRE(first.value); REQUIRE(blocks > 0); }
    { TempoAnalysisStore store(db.string()); blocks = 0; auto hit = analyze_snapshot_to_store(store, path, "shared-codec-test-v1", "default", epoch++, {}, [&](auto) { ++blocks; });
      INFO(hit.diagnostic); REQUIRE(hit.status == FileJobStatus::cache_hit); REQUIRE(hit.value); REQUIRE(blocks == 0); }
    auto after = fingerprint_file(path); REQUIRE(after.status == FingerprintStatus::ok); REQUIRE(after.digest == before.digest);
  }
}

TEST_CASE("snapshot job uses shared store and worker collection queries") {
  TempDirectory temp; auto input = temp.path / "audio"; auto fixtures = std::filesystem::path(BEATGLASS_FIXTURES);
  std::filesystem::copy_file(fixtures / "tone.aiff", input); auto db = temp.path / "library.sqlite";
  {
    TempoAnalysisStore concrete(db.string()); beatglass::AnalysisStore& store = concrete; int blocks = 0;
    auto first = analyze_snapshot_to_store(store, input, "shared-snapshot-v1", "old", 101, {}, [&](auto) { ++blocks; });
    INFO(first.diagnostic); REQUIRE(first.status == FileJobStatus::committed); REQUIRE(first.value); REQUIRE(blocks > 0);
    REQUIRE(first.value->result.observed_through.frame > 0); REQUIRE(first.value->result.observed_through.epoch == 101);
    CollectionEntry row; row.path = input.string(); row.title = "Fixture"; row.artist = "Example"; row.content_hash = first.value->identity.content_hash;
    concrete.put_entry(row); auto page = concrete.entries({"fixture", CollectionOrder::title, false, 10, 0});
    REQUIRE(page.size() == 1); REQUIRE(page[0].path == row.path); REQUIRE(concrete.entry(row.path));
    bool modified = false;
    auto changed = analyze_snapshot_to_store(store, input, "shared-snapshot-v1", "new", 102, {}, [&](auto) {
      if (!modified) { modified = true; std::ofstream out(input, std::ios::binary | std::ios::app); out << 'x'; }
    });
    INFO(changed.diagnostic); REQUIRE(modified); REQUIRE(changed.status == FileJobStatus::stale_input);
  }
  std::filesystem::copy_file(fixtures / "tone.aiff", input, std::filesystem::copy_options::overwrite_existing);
  {
    TempoAnalysisStore concrete(db.string()); beatglass::AnalysisStore& store = concrete; int blocks = 0;
    auto hit = analyze_snapshot_to_store(store, input, "shared-snapshot-v1", "old", 105, {}, [&](auto) { ++blocks; });
    INFO(hit.diagnostic); REQUIRE(hit.status == FileJobStatus::cache_hit); REQUIRE(hit.value); REQUIRE(hit.value->result.observed_through.epoch == 105); REQUIRE(blocks == 0);
    auto after = analyze_snapshot_to_store(store, input, "shared-snapshot-v1", "new", 103);
    INFO(after.diagnostic); REQUIRE(after.status == FileJobStatus::committed);
    bool replaced = false;
    auto stale = analyze_snapshot_to_store(store, input, "shared-snapshot-v1", "replacement", 106, {}, [&](auto) {
      if (!replaced) { replaced = true; std::filesystem::rename(input, temp.path / "old-file"); std::filesystem::copy_file(fixtures / "tone.aiff", input); }
    });
    REQUIRE(replaced); REQUIRE(stale.status == FileJobStatus::stale_input);
    std::filesystem::remove(input); std::filesystem::rename(temp.path / "old-file", input);
    REQUIRE(analyze_snapshot_to_store(store, input, "shared-snapshot-v1", "old", 107).status == FileJobStatus::cache_hit);
    std::stop_source in_flight;
    auto cancelled = analyze_snapshot_to_store(store, input, "shared-snapshot-v1", "cancel", 108, in_flight.get_token(), [&](auto) { in_flight.request_stop(); });
    REQUIRE(cancelled.status == FileJobStatus::cancelled);
    REQUIRE(analyze_snapshot_to_store(store, input, "shared-snapshot-v1", "cancel", 109).status == FileJobStatus::committed);
    std::stop_source stop; stop.request_stop();
    REQUIRE(analyze_snapshot_to_store(store, input, "shared-snapshot-v1", "new", 104, stop.get_token()).status == FileJobStatus::cancelled);
  }
}

TEST_CASE("shared store preserves cache when real snapshot verification is cancelled") {
  TempDirectory temp; auto input = temp.path / "audio";
  std::filesystem::copy_file(std::filesystem::path(BEATGLASS_FIXTURES) / "tone.aiff", input);
  TempoAnalysisStore store((temp.path / "library.sqlite").string());
  auto first = analyze_snapshot_to_store(store, input, "verify-v1", "config", 201);
  INFO(first.diagnostic); REQUIRE(first.status == FileJobStatus::committed); REQUIRE(first.value);
  auto snapshot = SnapshotOpen::create(input); INFO(snapshot.diagnostic); REQUIRE(snapshot.status == SnapshotStatus::ok);
  auto modified = snapshot.value->modified_unix_ns(); REQUIRE(modified);
  beatglass::FileVersion version{std::filesystem::absolute(input), snapshot.value->size(), *modified, snapshot.value->digest()};
  std::stop_source stop;
  class Verifier final : public beatglass::FileVersionVerifier {
  public:
    Verifier(FileSnapshot& s, std::stop_source& c) : snapshot(s), cancel(c) {}
    beatglass::FileVerificationResult verify(std::stop_token token) override {
      auto result = snapshot.verify(token, [&](auto) { cancel.request_stop(); });
      return {result == SnapshotStatus::cancelled ? beatglass::FileVerificationOutcome::cancelled : beatglass::FileVerificationOutcome::error, {}};
    }
    FileSnapshot& snapshot; std::stop_source& cancel;
  } verifier(*snapshot.value, stop);
  auto cancelled = store.publish(version, *first.value, "cancel-during-real-verify", verifier, stop.get_token());
  REQUIRE(cancelled.outcome == beatglass::PublishOutcome::cancelled);
  auto old = store.find(version, first.value->identity, {}); REQUIRE(old.outcome == beatglass::FindOutcome::hit); REQUIRE(old.result);
  REQUIRE(old.result->result.bpm == first.value->result.bpm);
}
