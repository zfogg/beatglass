#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/detail/file_job.hpp"
#include "beatglass/library/detail/job_executor.hpp"
#include <future>
#include <chrono>
#include <filesystem>
#include <fstream>
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
  std::filesystem::copy_file(fixtures / "tone.flac", input);
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
    std::filesystem::copy_file(fixtures / "tone.aiff", input, std::filesystem::copy_options::overwrite_existing);
    progress = 0;
    auto changed_file = analyze_cached_tempo(cache, input, "aubio-test-v1", "config1", {}, [&](auto) { ++progress; });
    INFO(changed_file.diagnostic); REQUIRE(changed_file.status == FileJobStatus::committed); REQUIRE(progress > 0);
  }
}
TEST_CASE("real tempo job cooperatively cancels and does not publish") {
  TempDirectory temp; auto input = temp.path / "audio";
  std::filesystem::copy_file(std::filesystem::path(BEATGLASS_FIXTURES) / "tone.flac", input);
  SqliteCache cache((temp.path / "library.sqlite").string()); std::stop_source stop;
  auto cancelled = analyze_cached_tempo(cache, input, "aubio-test-v1", "config", stop.get_token(), [&](auto) { stop.request_stop(); });
  REQUIRE(cancelled.status == FileJobStatus::cancelled);
  auto retry = analyze_cached_tempo(cache, input, "aubio-test-v1", "config");
  INFO(retry.diagnostic); REQUIRE(retry.status == FileJobStatus::committed);
}

TEST_CASE("real tempo job rejects a file changed during analysis") {
  TempDirectory temp; auto input = temp.path / "audio";
  std::filesystem::copy_file(std::filesystem::path(BEATGLASS_FIXTURES) / "tone.flac", input);
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
  std::filesystem::copy_file(std::filesystem::path(BEATGLASS_FIXTURES) / "tone.flac", input);
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
