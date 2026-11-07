#include "beatglass/library/detail/composite_snapshot_job.hpp"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
using namespace beatglass;
using namespace beatglass::library_detail;
namespace {
struct Directory {
  std::filesystem::path path=std::filesystem::temp_directory_path()/("beatglass-composite-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  Directory() { std::filesystem::create_directory(path); std::filesystem::copy_file(std::filesystem::path(BEATGLASS_FIXTURES)/"tone.aiff",path/"audio.aiff"); }
  ~Directory() { std::error_code ec; std::filesystem::remove_all(path,ec); }
};
}
TEST_CASE("snapshot music job returns bounded in-memory evidence without cache reuse") {
  Directory d; unsigned calls=0;
  auto value=analyze_snapshot_music(d.path/"audio.aiff",31,{1000,1000,100},{},[&](auto){++calls;});
  INFO(value.diagnostic); REQUIRE(value.status==CompositeJobStatus::complete); REQUIRE(value.value);
  CHECK(calls>0); CHECK(value.value->value.epoch==31); CHECK(value.value->value.observed_through>0);
  CHECK_FALSE(value.value->cache_reusable()); CHECK(value.value->value.configuration_id.empty());
  const auto first_calls=calls;
  auto again=analyze_snapshot_music(d.path/"audio.aiff",32,{1000,1000,100},{},[&](auto){++calls;});
  INFO(again.diagnostic); REQUIRE(again.status==CompositeJobStatus::complete); REQUIRE(again.value); CHECK(again.value->value.epoch==32); CHECK(calls>first_calls);
}
TEST_CASE("snapshot music worker discards cancelled or changed results") {
  Directory d; auto path=d.path/"audio.aiff"; std::stop_source stop;
  auto cancelled=analyze_snapshot_music(path,1,{1000,1000,100},stop.get_token(),[&](auto){stop.request_stop();});
  CHECK(cancelled.status==CompositeJobStatus::cancelled); CHECK_FALSE(cancelled.value);
  auto changed=analyze_snapshot_music(path,2,{1000,1000,100},{},[&](auto){std::ofstream out(path,std::ios::binary|std::ios::app);out.put('x');});
  CHECK(changed.status==CompositeJobStatus::stale_input); CHECK_FALSE(changed.value);
  auto invalid=analyze_snapshot_music(path,0,{1000,1000,100}); CHECK(invalid.status==CompositeJobStatus::error); CHECK_FALSE(invalid.value);
}
