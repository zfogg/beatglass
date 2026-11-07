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
  // Structural validity and caller-filled identity cannot establish loaded-code provenance.
  CHECK_FALSE(staged.cache_reusable());
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

TEST_CASE("collector limit breaches and source changes fail closed") {
  auto pcm=collector_audio();
  SECTION("beat count") {
    MusicCollector c(1,{0,1000,100});
    CHECK_THROWS_AS(c.process(input(pcm)),std::length_error);
    CHECK_THROWS(c.finish());
  }
  SECTION("each label byte limit") {
    MusicCollector c(1,{100,1000,0});
    CHECK_THROWS_AS(c.process(input(pcm)),std::length_error);
    CHECK_THROWS(c.finish());
  }
  SECTION("explicit discontinuity") {
    MusicCollector c(1,{100,1000,100});
    c.process(input(std::span(pcm).first(512)));
    auto block=input(std::span(pcm).subspan(512,512),512); block.discontinuity=true;
    CHECK_THROWS_AS(c.process(block),std::invalid_argument);
    CHECK_THROWS(c.finish());
  }
  SECTION("channel format change") {
    MusicCollector c(1,{100,1000,100});
    c.process(input(std::span(pcm).first(512)));
    auto block=input(std::span(pcm).subspan(512,512),512); block.format.channels=2;
    CHECK_THROWS(c.process(block));
    CHECK_THROWS(c.finish());
  }
}
TEST_CASE("collector output is independent of uneven block boundaries") {
  auto pcm=collector_audio();
  MusicCollector whole(1,{100,1000,100}), chunks(1,{100,1000,100});
  whole.process(input(pcm));
  for (std::size_t pos=0;pos<pcm.size();) {
    const auto count=std::min<std::size_t>(137,pcm.size()-pos);
    chunks.process(input(std::span(pcm).subspan(pos,count),pos)); pos+=count;
  }
  const auto a=whole.finish().value, b=chunks.finish().value;
  CHECK(a.observed_through==b.observed_through); CHECK(a.analyzed_through==b.analyzed_through);
  CHECK(a.sample_rate==b.sample_rate); CHECK(a.bpm==b.bpm);
  REQUIRE(a.beats.size()==b.beats.size());
  for (std::size_t i=0;i<a.beats.size();++i) {
    CHECK(a.beats[i].frame==b.beats[i].frame);
    CHECK(a.beats[i].observed_through==b.beats[i].observed_through);
    CHECK(a.beats[i].strength==b.beats[i].strength);
  }
  REQUIRE_FALSE(a.tonal_windows.empty()); REQUIRE(a.tonal_windows.size()==b.tonal_windows.size());
  for (std::size_t i=0;i<a.tonal_windows.size();++i) {
    const auto& x=a.tonal_windows[i]; const auto& y=b.tonal_windows[i];
    CHECK(x.start_frame==y.start_frame); CHECK(x.end_frame==y.end_frame);
    CHECK(x.rolling_key_from==y.rolling_key_from);
    REQUIRE(x.rolling_key.has_value()==y.rolling_key.has_value());
    REQUIRE(x.local_chord.has_value()==y.local_chord.has_value());
    if (x.rolling_key) { CHECK(x.rolling_key->label==y.rolling_key->label); CHECK(x.rolling_key->strength==y.rolling_key->strength); }
    if (x.local_chord) { CHECK(x.local_chord->label==y.local_chord->label); CHECK(x.local_chord->strength==y.local_chord->strength); }
  }
}
