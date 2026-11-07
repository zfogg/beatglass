#include "beatglass/core/visualization.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
using namespace beatglass;
TEST_CASE("visualization reports silence and sample boundaries") {
  VisualizationProcessor p; std::array<float,1024> x{};
  REQUIRE_FALSE(p.process(1, 20, 48000, std::span(x).first(512)));
  auto f = p.process(1, 532, 48000, std::span(x).last(512));
  REQUIRE(f); CHECK(f->first_frame == 20); CHECK(f->end_frame == 1044); CHECK(f->sample_rate == 48000);
  for (auto v : f->magnitude) CHECK(v == 0); for (auto v : f->maximum) CHECK(v == 0);
}
TEST_CASE("visualization locates an FFT tone and preserves waveform extrema") {
  VisualizationProcessor p; std::array<float,1024> x{};
  for (unsigned i=0;i<x.size();++i) x[i]=float(std::sin(2*std::numbers::pi*32*i/1024));
  auto f=p.process(9,0,48000,x); REQUIRE(f); CHECK(f->magnitude[32] == Catch::Approx(1).margin(.002));
  CHECK(f->magnitude[80] < .001f); CHECK(f->maximum[0] == Catch::Approx(1).margin(.001)); CHECK(f->minimum[1] < 0);
}
TEST_CASE("visualization discards partial windows after reset, gaps and source changes") {
  VisualizationProcessor p; std::array<float,512> x{};
  REQUIRE_FALSE(p.process(1,0,44100,x)); REQUIRE_FALSE(p.process(1,700,44100,x));
  REQUIRE_FALSE(p.process(2,1212,44100,x)); REQUIRE_FALSE(p.process(2,1724,48000,x));
  p.reset(); REQUIRE_FALSE(p.process(2,2236,48000,x)); auto f=p.process(2,2748,48000,x); REQUIRE(f); CHECK(f->first_frame==2236);
}
TEST_CASE("visualization rejects invalid input without mutating partial state") {
  VisualizationProcessor p; std::array<float,512> x{}; REQUIRE_FALSE(p.process(1,0,48000,x));
  x[0]=std::numeric_limits<float>::quiet_NaN(); CHECK_THROWS(p.process(1,512,48000,x)); x[0]=0;
  REQUIRE(p.process(1,512,48000,x)); CHECK_THROWS(p.process(0,0,48000,x));
}
TEST_CASE("visualization bounds work per call") {
  VisualizationProcessor p; std::array<float,4097> too_large{};
  CHECK_THROWS(p.process(1,0,48000,too_large));
}

