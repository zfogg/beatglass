#include "beatglass/core/music_collector.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <numbers>
#include <vector>
using namespace beatglass;
namespace {
std::vector<float> collector_audio() {
  std::vector<float> pcm(44100*3);
  for (std::size_t i=0;i<pcm.size();++i) {
    const double t=2*std::numbers::pi*i/44100;
    const auto phase=i%(44100/2);
    pcm[i]=static_cast<float>(.15*(std::sin(261.625565*t)+std::sin(329.627557*t)+std::sin(391.995436*t)) + (phase<1000 ? .6*std::exp(-double(phase)/100)*std::sin(double(phase)*.4) : 0));
  }
  return pcm;
}
AudioBlock input(std::span<const float> samples, std::uint64_t first=0, std::uint64_t epoch=1) {
  return {{44100,1},epoch,first,0,false,samples};
}
}
TEST_CASE("bounded music collector owns causal events and refuses cache reuse") {
  auto pcm=collector_audio(); MusicCollector collector(1,{100,1000,100});
  collector.process(input(pcm)); auto staged=collector.finish(); auto& result=staged.value;
  CHECK_FALSE(staged.cache_reusable()); CHECK(result.epoch==1); CHECK(result.sample_rate==44100);
  CHECK(result.observed_through==pcm.size()); CHECK(result.analyzed_through<=result.observed_through);
  CHECK_FALSE(result.validated_track_key); REQUIRE_FALSE(result.tonal_windows.empty());
  CHECK(result.engine_version.empty()); CHECK(result.configuration_id.empty()); CHECK(result.algorithms.empty());
  CHECK(validate_offline_analysis(result,{100,1000,10,100})==OfflineValidation::invalid_metadata);
  // Temporary fixture identity exercises structural mapping only, not provenance.
  result.engine_version="fixture"; result.configuration_id="fixture"; result.algorithms.push_back({"fixture","fixture"});
  CHECK(validate_offline_analysis(result,{100,1000,10,100})==OfflineValidation::valid);
  CHECK_THROWS(collector.finish()); CHECK_THROWS(collector.process(input(pcm)));
}
TEST_CASE("collector limits and discontinuities invalidate all staged output") {
  auto pcm=collector_audio();
  CHECK_THROWS_AS(MusicCollector(0,{100,1000,100}),std::invalid_argument);
  MusicCollector limited(1,{100,0,100}); CHECK_THROWS_AS(limited.process(input(pcm)),std::length_error); CHECK_THROWS(limited.finish());
  MusicCollector gap(1,{100,1000,100}); gap.process(input(std::span(pcm).first(512))); CHECK_THROWS(gap.process(input(std::span(pcm).first(512),513))); CHECK_THROWS(gap.finish());
  MusicCollector wrong_epoch(1,{100,1000,100}); CHECK_THROWS(wrong_epoch.process(input(pcm,0,2))); CHECK_THROWS(wrong_epoch.finish());
  MusicCollector empty(1,{100,1000,100}); CHECK_THROWS(empty.finish());
}
