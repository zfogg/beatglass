#include "beatglass/core/music_replay.hpp"
#include "beatglass/capture/file_reader.hpp"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <stop_token>
#include <numbers>
#include <vector>
using namespace beatglass;
namespace {
std::vector<float> music(unsigned seconds) {
  constexpr unsigned rate = 44100;
  std::vector<float> pcm(rate * seconds);
  for (std::size_t i=0; i<pcm.size(); ++i) {
    const double t=2*std::numbers::pi*i/rate;
    const auto phase=i%(rate/2);
    const double click=phase<1000 ? .6*std::exp(-double(phase)/100)*std::sin(double(phase)*.4) : 0;
    pcm[i]=static_cast<float>(.15*(std::sin(261.625565*t)+std::sin(329.627557*t)+std::sin(391.995436*t))+click);
  }
  return pcm;
}
AudioBlock block(std::span<const float> samples, std::uint64_t epoch, std::uint64_t first, bool discontinuity=false) {
  return {{44100, 1}, epoch, first, 0, discontinuity, samples};
}
}
TEST_CASE("combined real estimators consume causal blocks independent of chunking") {
  auto pcm=music(20); CausalMusicReplay whole, chunks; std::vector<CausalMusicBeat> a,b; std::vector<CausalTonalFrame> ta,tb;
  const auto one=whole.process(block(pcm, 5, 100), [&](const CausalMusicBeat& e) { a.push_back(e); }, [&](const CausalTonalFrame& t) { ta.push_back(t); });
  CausalMusicState many;
  for (std::size_t i=0; i<pcm.size(); i+=137) {
    const auto n=std::min<std::size_t>(137,pcm.size()-i);
    many=chunks.process(block(std::span(pcm).subspan(i,n),5,100+i), [&](const CausalMusicBeat& e) { b.push_back(e); }, [&](const CausalTonalFrame& t) { tb.push_back(t); });
  }
  CHECK(one.sample_rate==44100); REQUIRE(one.tempo.bpm); REQUIRE(one.rolling_key); REQUIRE(one.local_chord); REQUIRE(one.rolling_key_strength); REQUIRE(one.local_chord_strength); CHECK(*one.local_chord == "C major");
  CHECK(many.tempo.bpm == one.tempo.bpm); CHECK(many.rolling_key == one.rolling_key); CHECK(many.local_chord == one.local_chord);
  CHECK(many.tonal_through == one.tonal_through); REQUIRE(one.tonal_through); CHECK(*one.tonal_through <= one.tempo.analyzed_through);
  REQUIRE_FALSE(ta.empty()); REQUIRE(ta.size()==tb.size());
  for (std::size_t i=0; i<ta.size(); ++i) { CHECK(ta[i].epoch==5); CHECK(ta[i].sample_rate==44100); CHECK(ta[i].first_frame<ta[i].observed_through); REQUIRE(ta[i].rolling_key_from); CHECK(*ta[i].rolling_key_from>=100); CHECK(*ta[i].rolling_key_from<=ta[i].first_frame); CHECK(ta[i].first_frame==tb[i].first_frame); CHECK(ta[i].observed_through==tb[i].observed_through); CHECK(ta[i].local_chord==tb[i].local_chord); }
  REQUIRE(one.tonal_from); REQUIRE(one.rolling_key_from); CHECK(*one.rolling_key_from <= *one.tonal_from); CHECK(*one.tonal_from < *one.tonal_through);
  REQUIRE_FALSE(a.empty()); REQUIRE(a.size()==b.size());
  for (std::size_t i=0; i<a.size(); ++i) { CHECK(a[i].epoch==5); CHECK(a[i].sample_rate==44100); CHECK(a[i].frame<a[i].observed_through); CHECK_FALSE(a[i].strength); CHECK(a[i].frame==b[i].frame); CHECK(a[i].observed_through==b[i].observed_through); }
}
TEST_CASE("combined replay invalidates old evidence on gap silence and resume") {
  auto pcm=music(20); CausalMusicReplay replay;
  const auto acquired=replay.process(block(pcm,1,0)); REQUIRE(acquired.rolling_key); REQUIRE(acquired.local_chord); REQUIRE(acquired.tempo.bpm);
  const auto gap=replay.process(block(std::span(pcm).first(100),1,pcm.size()+50));
  CHECK_FALSE(gap.rolling_key); CHECK_FALSE(gap.local_chord); CHECK_FALSE(gap.rolling_key_strength); CHECK_FALSE(gap.local_chord_strength); CHECK_FALSE(gap.tempo.bpm); CHECK_FALSE(gap.tonal_through);
  const auto fresh=replay.process(block(pcm,2,0)); REQUIRE(fresh.rolling_key);
  std::vector<float> silence(44100*2); const auto quiet=replay.process(block(silence,2,pcm.size()));
  CHECK(quiet.tempo.state==TrackingState::silent); CHECK_FALSE(quiet.rolling_key); CHECK_FALSE(quiet.local_chord); CHECK_FALSE(quiet.tempo.bpm);
  const auto resumed=replay.process(block(std::span(pcm).first(512),2,pcm.size()+silence.size()));
  CHECK_FALSE(resumed.rolling_key); CHECK_FALSE(resumed.local_chord); CHECK_FALSE(resumed.tempo.bpm);
  std::vector<float> invalid{std::numeric_limits<float>::quiet_NaN()}; CHECK_THROWS(replay.process(block(invalid,3,0)));
  CHECK(replay.state().tempo.state==TrackingState::error); CHECK_FALSE(replay.state().rolling_key);
}
TEST_CASE("interruption clears combined state and requires a fresh epoch") {
  auto pcm=music(20); CausalMusicReplay replay; REQUIRE(replay.process(block(pcm,40,0)).rolling_key);
  replay.interrupt(); CHECK(replay.state().tempo.state==TrackingState::stopped); CHECK_FALSE(replay.state().rolling_key);
  const auto fresh=replay.process(block(std::span(pcm).first(100),41,0)); CHECK_FALSE(fresh.rolling_key); CHECK_FALSE(fresh.tempo.bpm);
  CausalMusicReplay stale; REQUIRE(stale.process(block(pcm,40,0)).rolling_key); stale.interrupt();
  CHECK_THROWS(stale.process(block(std::span(pcm).first(100),40,0))); CHECK(stale.state().tempo.state==TrackingState::error);
}
TEST_CASE("file replay and incremental blocks use the same real music engine") {
  struct TempWave {
    std::filesystem::path path;
    explicit TempWave(const std::vector<float>& audio) {
      std::random_device rng;
      path=std::filesystem::temp_directory_path() / ("beatglass-music-"+std::to_string(rng())+"-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".wav");
      if (std::filesystem::exists(path)) throw std::runtime_error("temporary fixture exists");
      std::ofstream out(path,std::ios::binary);
      auto u16=[&](unsigned v){char x[2]={char(v&255),char((v>>8)&255)};out.write(x,2);};
      auto u32=[&](unsigned v){u16(v&65535);u16(v>>16);};
      out.write("RIFF",4);u32(36+static_cast<unsigned>(audio.size()*2));out.write("WAVEfmt ",8);u32(16);u16(1);u16(1);u32(44100);u32(88200);u16(2);u16(16);out.write("data",4);u32(static_cast<unsigned>(audio.size()*2));
      for (auto sample:audio) { const auto n=static_cast<int>(std::clamp(sample,-1.f,0.999f)*32767); u16(static_cast<unsigned>(n)&65535); }
      if(!out) throw std::runtime_error("fixture write failed");
    }
    ~TempWave(){std::error_code e;std::filesystem::remove(path,e);}
  } fixture(music(12));
  auto contents=[&]{std::ifstream in(fixture.path,std::ios::binary);return std::vector<char>(std::istreambuf_iterator<char>(in),{});};
  const auto before=contents(); REQUIRE_FALSE(before.empty());
  bool called=false; TempWave empty(std::vector<float>{});
  CHECK_THROWS_AS(replay_music_file(empty.path,0,[&](const CausalMusicBeat&){called=true;}),std::invalid_argument);
  CHECK_THROWS_AS(replay_music_file(empty.path / "missing",0),std::invalid_argument);
  CHECK_FALSE(called); CHECK(replay_music_file(empty.path,76).tempo.epoch==76);
  std::vector<CausalMusicBeat> file_beats, block_beats;
  const auto file=replay_music_file(fixture.path,72,[&](const CausalMusicBeat& e){file_beats.push_back(e);});
  FileReader reader(fixture.path,72); CausalMusicReplay replay; std::array<float,512> samples{}; CausalMusicState blocks;
  for (;;) { auto r=reader.read(samples); if(r.status==FileReadStatus::end) break; REQUIRE(r.status==FileReadStatus::data); blocks=replay.process(r.block,[&](const CausalMusicBeat& e){block_beats.push_back(e);}); }
  REQUIRE(file.rolling_key); REQUIRE(file.local_chord); REQUIRE(file.tempo.bpm); REQUIRE_FALSE(file_beats.empty());
  CHECK(file.rolling_key==blocks.rolling_key); CHECK(file.local_chord==blocks.local_chord); CHECK(file.tempo.bpm==blocks.tempo.bpm);
  REQUIRE(file_beats.size()==block_beats.size()); for(std::size_t i=0;i<file_beats.size();++i) CHECK(file_beats[i].frame==block_beats[i].frame);
  CHECK(contents()==before);
  std::stop_source cancelled; cancelled.request_stop(); CHECK_THROWS_AS(replay_music_file(fixture.path,73,{},cancelled.get_token()),MusicReplayCancelled);
  std::stop_source during; CHECK_THROWS_AS(replay_music_file(fixture.path,74,[&](const CausalMusicBeat&){during.request_stop();},during.get_token()),MusicReplayCancelled);
  std::stop_source after_block; unsigned updates=0;
  CHECK_THROWS_AS(replay_music_file(fixture.path,75,{},after_block.get_token(),{},[&](const CausalMusicState& s){++updates; CHECK(s.tempo.observed_through==512); after_block.request_stop();}),MusicReplayCancelled);
  CHECK(updates==1);
}
TEST_CASE("combined callback failure clears state and fails closed") {
  auto pcm=music(20); CausalMusicReplay replay;
  CHECK_THROWS(replay.process(block(pcm,9,0),[](const CausalMusicBeat&){throw std::runtime_error("callback");}));
  CHECK(replay.state().tempo.state==TrackingState::error); CHECK_FALSE(replay.state().rolling_key); CHECK_FALSE(replay.state().local_chord);
  CHECK_THROWS(replay.process(block(std::span(pcm).first(512),10,0)));
}

TEST_CASE("file replay rejects source mutation before publication") {
  auto pcm = music(4);
  struct Fixture { std::filesystem::path path; Fixture(const std::vector<float>& audio) { path=std::filesystem::temp_directory_path()/"beatglass-revalidate.wav"; std::ofstream out(path,std::ios::binary); auto u16=[&](unsigned v){char x[2]={char(v&255),char((v>>8)&255)};out.write(x,2);}; auto u32=[&](unsigned v){u16(v&65535);u16(v>>16);}; out.write("RIFF",4);u32(36+static_cast<unsigned>(audio.size()*2));out.write("WAVEfmt ",8);u32(16);u16(1);u16(1);u32(44100);u32(88200);u16(2);u16(16);out.write("data",4);u32(static_cast<unsigned>(audio.size()*2));for(auto sample:audio){auto n=static_cast<int>(std::clamp(sample,-1.f,.999f)*32767);u16(static_cast<unsigned>(n)&65535);} } ~Fixture(){std::error_code e;std::filesystem::remove(path,e);} } fixture(pcm);
  bool mutated=false;
  CHECK_THROWS_AS(replay_music_file(fixture.path,91,{}, {}, {}, [&](const CausalMusicState&) { if (!mutated) { std::ofstream out(fixture.path,std::ios::binary|std::ios::app); out << 'x'; mutated=true; } }), std::runtime_error);
  CHECK(mutated);
}

TEST_CASE("combined replay rejects zero epoch before observers") {
  auto pcm=music(1); CausalMusicReplay replay; bool called=false;
  CHECK_THROWS_AS(replay.process(block(pcm,0,0),[&](const CausalMusicBeat&){called=true;},[&](const CausalTonalFrame&){called=true;}),std::invalid_argument);
  CHECK_FALSE(called); CHECK(replay.state().tempo.state==TrackingState::error);
  CHECK_THROWS(replay.process(block(pcm,1,0)));
}
