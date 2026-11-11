#include "beatglass/core/visualization_color_mapping.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
using namespace beatglass;
TEST_CASE("visualization color mapping normalizes energy and selects palette endpoints") {
  VisualizationColorMapping m; const auto low=map_visualization_energy({1,0,0},m); const auto mid=map_visualization_energy({0,1,0},m); const auto high=map_visualization_energy({0,0,1},m);
  CHECK(low.red==Catch::Approx(m.low.red)); CHECK(mid.blue==Catch::Approx(m.mid.blue)); CHECK(high.green==Catch::Approx(m.high.green));
  CHECK(map_visualization_energy({2,0,0},m).red==Catch::Approx(low.red));
}
TEST_CASE("visualization color mapping is stable for silence and protects contrast") {
  VisualizationColorMapping m; m.background={.5f,.5f,.5f}; m.minimum_contrast=2.0f; const auto a=map_visualization_energy({},m); const auto b=map_visualization_energy({0,0,0},m);
  CHECK(a.red==Catch::Approx(b.red)); CHECK(visualization_contrast(a,m.background)>=2.0f);
}
