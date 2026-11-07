#include "beatglass/core/visualization.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
using namespace beatglass;
struct Gate {
  std::uint64_t generation = 1, revision = 0, seen = 0;
  unsigned clears = 0;
  bool visible = false;
  static void clear(void* ptr) noexcept { auto& g=*static_cast<Gate*>(ptr); ++g.generation; ++g.revision; ++g.clears; g.seen=g.revision; g.visible=false; }
  bool apply(std::uint64_t gen, std::uint64_t rev) { if (gen != generation || rev <= seen) return false; seen=rev; visible=true; return true; }
};
TEST_CASE("visualization reports silence and sample boundaries") {
  Gate gate; VisualizationProcessor p(Gate::clear, &gate); std::array<float,1024> x{};
  REQUIRE_FALSE(p.process(1, 20, 48000, std::span(x).first(512)));
  auto f = p.process(1, 532, 48000, std::span(x).last(512));
  REQUIRE(f); CHECK(f->first_frame == 20); CHECK(f->end_frame == 1044); CHECK(f->sample_rate == 48000);
  for (auto v : f->magnitude) CHECK(v == 0); for (auto v : f->maximum) CHECK(v == 0);
}
TEST_CASE("visualization locates an FFT tone and preserves waveform extrema") {
  Gate gate; VisualizationProcessor p(Gate::clear, &gate); std::array<float,1024> x{};
  for (unsigned i=0;i<x.size();++i) x[i]=float(std::sin(2*std::numbers::pi*32*i/1024));
  auto f=p.process(9,0,48000,x); REQUIRE(f); CHECK(f->magnitude[32] == Catch::Approx(1).margin(.002));
  CHECK(f->magnitude[80] < .001f); CHECK(f->maximum[0] == Catch::Approx(1).margin(.001)); CHECK(f->minimum[1] < 0);
}
TEST_CASE("visualization discards partial windows after reset, gaps and source changes") {
  Gate gate; VisualizationProcessor p(Gate::clear, &gate); std::array<float,512> x{};
  REQUIRE_FALSE(p.process(1,0,44100,x)); REQUIRE_FALSE(p.process(1,700,44100,x));
  REQUIRE_FALSE(p.process(2,1212,44100,x)); CHECK_THROWS(p.process(2,1724,48000,x));
  p.reset(); REQUIRE_FALSE(p.process(3,2236,48000,x)); auto f=p.process(3,2748,48000,x); REQUIRE(f); CHECK(f->first_frame==2236);
}
TEST_CASE("visualization invalid input clears partial state") {
  Gate gate; VisualizationProcessor p(Gate::clear, &gate); std::array<float,512> x{}; REQUIRE_FALSE(p.process(1,0,48000,x));
  x[0]=std::numeric_limits<float>::quiet_NaN(); CHECK_THROWS(p.process(1,512,48000,x)); x[0]=0;
  REQUIRE_FALSE(p.process(1,512,48000,x)); CHECK_THROWS(p.process(0,0,48000,x));
}
TEST_CASE("visualization bounds work per call") {
  Gate gate; VisualizationProcessor p(Gate::clear, &gate); std::array<float,4097> too_large{};
  CHECK_THROWS(p.process(1,0,48000,too_large));
}


TEST_CASE("visualization invalidation rejects delayed same-epoch and reordered frames") {
  Gate gate; VisualizationProcessor p(Gate::clear, &gate); std::array<float,1024> x{};
  REQUIRE(p.process(1,0,48000,x)); auto oldgen=gate.generation; auto oldrev=++gate.revision;
  REQUIRE_FALSE(p.process(1,1200,48000,std::span(x).first(512)));
  CHECK_FALSE(gate.apply(oldgen,oldrev)); CHECK_FALSE(gate.visible);
  REQUIRE(p.process(1,1712,48000,std::span(x).first(512))); auto gen=gate.generation; auto rev=++gate.revision;
  CHECK(gate.apply(gen,rev)); auto newer=++gate.revision; CHECK(gate.apply(gen,newer)); CHECK_FALSE(gate.apply(gen,rev));
  CHECK_THROWS(p.process(1,2000,48000,std::span(x).first(512))); CHECK_FALSE(gate.apply(gen,newer)); CHECK_FALSE(gate.visible);
  auto before=gate.generation; p.reset(); CHECK(gate.generation>before);
}

TEST_CASE("contiguous explicit discontinuity invalidates old window") {
  Gate gate; VisualizationProcessor p(Gate::clear, &gate); std::array<float,1024> x{};
  REQUIRE(p.process(1,0,48000,x)); auto gen=gate.generation; auto rev=++gate.revision;
  REQUIRE_FALSE(p.process(1,1024,48000,std::span(x).first(512),true)); CHECK_FALSE(gate.apply(gen,rev));
  auto f=p.process(1,1536,48000,std::span(x).first(512)); REQUIRE(f); CHECK(f->first_frame==1024);
}
TEST_CASE("visualization high-water clear contract is synchronous") {
  Gate gate; VisualizationProcessor p(Gate::clear, &gate); std::array<float,512> x{};
  REQUIRE_FALSE(p.process(1, 0, 48000, x));
  CHECK(gate.clears == 0);
  REQUIRE_FALSE(p.process(1, 512, 48000, x, true));
  CHECK(gate.clears == 1);
  const auto generation = gate.generation;
  CHECK_THROWS(p.process(1, 900, 48000, x));
  CHECK(gate.clears == 2);
  CHECK(gate.generation > generation);
  const auto before_reset = gate.clears;
  p.reset();
  CHECK(gate.clears == before_reset + 1);
}
TEST_CASE("chunking and latest-only windows preserve timing and magnitudes") {
  Gate a,b; VisualizationProcessor full(Gate::clear,&a), chunked(Gate::clear,&b); std::array<float,4096> x{};
  for (unsigned i=0;i<x.size();++i) x[i]=float(.5*std::sin(2*std::numbers::pi*16*i/1024));
  auto f=full.process(1,0,44100,x); REQUIRE(f); CHECK(f->first_frame==3072); CHECK(f->end_frame==4096);
  std::optional<VisualizationFrame> last; unsigned pos=0;
  while(pos<x.size()) { auto n=std::min<unsigned>(137, unsigned(x.size()-pos)); auto r=chunked.process(1,pos,44100,std::span(x).subspan(pos,n)); if(r) last=r; pos+=n; }
  REQUIRE(last); CHECK(last->first_frame==f->first_frame); CHECK(last->magnitude[16]==Catch::Approx(f->magnitude[16]));
  std::array<float,100> tail{}; CHECK_FALSE(full.process(1,4096,44100,tail));
}
TEST_CASE("FFT DC and Nyquist normalization and invalid range") {
  Gate gate; VisualizationProcessor p(Gate::clear,&gate); std::array<float,1024> x{}; x.fill(.5f);
  auto dc=p.process(1,0,48000,x); REQUIRE(dc); CHECK(dc->magnitude[0]==Catch::Approx(.5f).margin(.001));
  for(unsigned i=0;i<x.size();++i) x[i]=(i%2)?-.75f:.75f;
  auto ny=p.process(1,1024,48000,x); REQUIRE(ny); CHECK(ny->magnitude[512]==Catch::Approx(.75f).margin(.001));
  x[0]=1.1f; auto old=gate.generation; CHECK_THROWS(p.process(1,2048,48000,x)); CHECK(gate.generation>old);
  x[0]=0; CHECK_THROWS(p.process(1,std::numeric_limits<std::uint64_t>::max()-1,48000,x));
}

TEST_CASE("stale epochs and overlaps remain rejected across reset") {
  Gate gate; VisualizationProcessor p(Gate::clear,&gate); std::array<float,1024> x{};
  REQUIRE(p.process(5,0,48000,x)); auto oldgen=gate.generation; auto oldrev=++gate.revision;
  CHECK_THROWS(p.process(4,1024,48000,x)); CHECK_FALSE(gate.apply(oldgen,oldrev));
  CHECK_THROWS(p.process(5,512,48000,x)); p.reset(); CHECK_THROWS(p.process(4,2048,48000,x));
  CHECK_THROWS(p.process(5,0,48000,x)); REQUIRE(p.process(6,0,48000,x));
  CHECK_THROWS(p.process(5,2048,48000,x));
}

TEST_CASE("sample rate cannot change within an epoch even after reset") {
  Gate gate; VisualizationProcessor p(Gate::clear,&gate); std::array<float,512> x{};
  REQUIRE_FALSE(p.process(1,0,44100,x)); auto old=gate.generation;
  CHECK_THROWS(p.process(1,512,48000,x)); CHECK(gate.generation>old);
  p.reset(); CHECK_THROWS(p.process(1,512,48000,x));
  REQUIRE_FALSE(p.process(2,0,48000,x)); REQUIRE(p.process(2,512,48000,x));
}
