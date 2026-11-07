#include "beatglass/core/causal_replay.hpp"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace beatglass;
namespace {
std::vector<float> click_pcm(unsigned rate, std::size_t seconds, unsigned channels = 1) {
  std::vector<float> audio(rate * seconds * channels);
  for (std::size_t i=0; i<rate * seconds; ++i) {
    const auto phase = i % (rate/2);
    const auto sample = phase < 1000 ? static_cast<float>(.8 * std::exp(-double(phase)/100) * std::sin(double(phase)*.4)) : 0.f;
    for (unsigned c=0; c<channels; ++c) audio[i*channels+c] = c ? -sample : sample;
  }
  return audio;
}
AudioBlock block(std::span<const float> samples, std::uint64_t epoch, std::uint64_t first, unsigned rate=44100, unsigned channels=1, bool discontinuity=false) {
  return {{rate, static_cast<std::uint16_t>(channels)}, epoch, first, 0, discontinuity, samples};
}
}
TEST_CASE("causal tempo replay is chunk invariant and owns partial hop data") {
  for (unsigned rate : {44100U, 48000U}) {
    const auto audio = click_pcm(rate, 20, 2);
    CausalTempoReplay whole, chunks;
    auto expected = whole.process(block(audio, 1, 100, rate, 2));
    CausalTempoState actual;
    const auto frames = audio.size()/2;
    for (std::size_t i=0; i<frames; i+=137) {
      const auto n = std::min<std::size_t>(137, frames-i);
      auto owned = std::vector<float>(audio.begin()+i*2, audio.begin()+(i+n)*2);
      actual = chunks.process(block(owned, 1, 100+i, rate, 2));
      std::fill(owned.begin(), owned.end(), 0.f);
      CHECK(actual.observed_through == 100+i+n);
      CHECK(actual.analyzed_through <= actual.observed_through);
    }
    REQUIRE(expected.bpm); REQUIRE(actual.bpm);
    CHECK(actual.bpm == expected.bpm); CHECK(actual.beats == expected.beats);
    CHECK(actual.state == expected.state); CHECK(actual.analyzed_through == expected.analyzed_through);
    CHECK(actual.observed_through == 100+frames);
  }
}
TEST_CASE("causal tempo replay clears stale evidence and reacquires after gaps and silence") {
  const auto audio = click_pcm(44100, 20); CausalTempoReplay replay;
  auto acquired = replay.process(block(audio, 5, 0)); REQUIRE(acquired.bpm); REQUIRE(acquired.state == TrackingState::tracking);
  auto reset = replay.process(block(std::span(audio).first(100), 5, audio.size(), 44100, 1, true));
  CHECK_FALSE(reset.bpm); CHECK(reset.beats == 0); CHECK(reset.state == TrackingState::reacquiring);
  auto gap = replay.process(block(std::span(audio).first(100), 5, audio.size()+110));
  CHECK_FALSE(gap.bpm); CHECK(gap.beats == 0); CHECK(gap.state == TrackingState::reacquiring);
  CHECK(gap.analyzed_through == audio.size()+110); CHECK(gap.observed_through == audio.size()+210);
  auto next = replay.process(block(audio, 6, 0)); REQUIRE(next.bpm);
  std::vector<float> silence(44100*2); auto quiet = replay.process(block(silence, 6, audio.size()));
  CHECK_FALSE(quiet.bpm); CHECK(quiet.state == TrackingState::silent);
  auto resumed = replay.process(block(std::span(audio).first(512), 6, audio.size()+silence.size()));
  CHECK_FALSE(resumed.bpm); CHECK(resumed.state == TrackingState::acquiring);
}
TEST_CASE("causal tempo replay rejects invalid blocks and fails closed") {
  auto audio = click_pcm(44100, 1); CausalTempoReplay replay;
  replay.process(block(audio, 1, 0));
  CHECK_THROWS(replay.process(block(std::span(audio).first(10), 1, audio.size(), 48000)));
  CHECK(replay.state().state == TrackingState::error); CHECK_FALSE(replay.state().bpm);
  CHECK_THROWS(replay.process(block(audio, 2, 0)));
  CausalTempoReplay invalid; std::vector<float> bad{std::numeric_limits<float>::quiet_NaN()};
  CHECK_THROWS(invalid.process(block(bad, 1, 0))); CHECK(invalid.state().state == TrackingState::error);
  CausalTempoReplay overflow; CHECK_THROWS(overflow.process(block(audio, 1, std::numeric_limits<std::uint64_t>::max()-1)));
}
TEST_CASE("causal tempo replay rejects old epochs and overlapping blocks") {
  auto audio = click_pcm(44100, 1);
  CausalTempoReplay old; old.process(block(std::span(audio).first(512), 10, 0));
  old.process(block(std::span(audio).first(512), 11, 0));
  CHECK_THROWS(old.process(block(std::span(audio).first(512), 10, 512)));
  CHECK(old.state().state == TrackingState::error);
  CausalTempoReplay overlap; overlap.process(block(std::span(audio).first(512), 12, 0));
  CHECK_THROWS(overlap.process(block(std::span(audio).first(512), 12, 256)));
  CHECK(overlap.state().state == TrackingState::error);
}
TEST_CASE("causal tempo replay permits a new format only in a new epoch") {
  auto a = click_pcm(44100, 1); auto b = click_pcm(48000, 1);
  CausalTempoReplay replay; replay.process(block(a, 3, 0, 44100));
  const auto changed = replay.process(block(b, 4, 0, 48000));
  CHECK(changed.epoch == 4); CHECK(changed.observed_through == b.size());
  CHECK(changed.analyzed_through <= changed.observed_through);
}
TEST_CASE("hop observers are synchronous borrowed and independent of source chunking") {
  auto pcm=click_pcm(44100,1,2); const auto frames=pcm.size()/2;
  struct Copy { std::uint64_t epoch,first; std::uint32_t rate; bool reset; std::vector<float> samples; };
  std::vector<Copy> single,split; CausalTempoReplay one,many;
  auto copy=[](std::vector<Copy>& out,const CausalMonoHop& h) { out.push_back({h.epoch,h.first_frame,h.sample_rate,h.reset,{h.samples.begin(),h.samples.end()}}); };
  bool inside=false; auto token=std::make_shared<int>(1); std::weak_ptr<int> weak=token;
  {
    CausalTempoReplay::HopObserver observer=[&,owned=token](const CausalMonoHop& h,TrackingState) { CHECK(inside); CHECK(h.samples.size()==512); copy(single,h); };
    token.reset(); inside=true; one.process(block(pcm,9,37,44100,2),observer); inside=false;
    CHECK_FALSE(weak.expired());
  }
  CHECK(weak.expired());
  for (std::size_t i=0;i<frames;i+=137) {
    auto n=std::min<std::size_t>(137,frames-i);
    auto temporary=std::vector<float>(pcm.begin()+i*2,pcm.begin()+(i+n)*2);
    many.process(block(temporary,9,37+i,44100,2),[&](const CausalMonoHop& h,TrackingState){copy(split,h);});
    std::fill(temporary.begin(),temporary.end(),0.f);
  }
  REQUIRE_FALSE(single.empty()); REQUIRE(single.size()==split.size());
  for (std::size_t i=0;i<single.size();++i) {
    CHECK(single[i].epoch==9); CHECK(single[i].first==37+i*512); CHECK(single[i].rate==44100);
    CHECK(single[i].reset==(i==0)); CHECK(split[i].first==single[i].first);
    CHECK(split[i].reset==single[i].reset); CHECK(split[i].samples==single[i].samples);
    for (std::size_t sample=0;sample<single[i].samples.size();++sample)
      CHECK(single[i].samples[sample]==pcm[(i*512+sample)*2]);
  }
}
TEST_CASE("hop observer exceptions and reentrant process fail closed") {
  auto pcm=click_pcm(44100,1); auto input=block(std::span(pcm).first(512),12,0);
  CausalTempoReplay throwing;
  CHECK_THROWS_AS(throwing.process(input,[](const CausalMonoHop&,TrackingState){throw std::logic_error("observer");}),std::logic_error);
  CHECK(throwing.state().state==TrackingState::error); CHECK_FALSE(throwing.state().bpm); CHECK_THROWS(throwing.process(input));
  CausalTempoReplay nested; bool rejected=false;
  CHECK_THROWS_AS(nested.process(input,[&](const CausalMonoHop&,TrackingState){
    try { nested.process(input); } catch (const std::logic_error&) { rejected=true; }
  }),std::logic_error);
  CHECK(rejected); CHECK(nested.state().state==TrackingState::error); CHECK_THROWS(nested.process(input));
}
TEST_CASE("hop reset signals reacquisition silence and resume") {
  CausalTempoReplay replay; std::vector<float> active(512,0.1f), quiet(512*90,0.f);
  std::vector<bool> resets; auto observe=[&](const CausalMonoHop& hop,TrackingState){resets.push_back(hop.reset);};
  replay.process(block(active,4,0),observe);
  replay.process(block(active,4,600),observe);
  replay.process(block(quiet,4,1112),observe);
  replay.process(block(active,4,1112+quiet.size()),observe);
  REQUIRE(resets.size()==93); CHECK(resets[0]); CHECK(resets[1]); CHECK(resets.back());
  CHECK(std::count(resets.begin()+2,resets.end()-1,true)==1);
}
