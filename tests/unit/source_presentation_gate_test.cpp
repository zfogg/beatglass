#include <catch2/catch_test_macros.hpp>
#include "beatglass/core/source_presentation_gate.hpp"
#include <memory>
using namespace beatglass;

TEST_CASE("source generation rejects delayed frames after clear and restart") {
  SourcePresentationGate gate;
  auto first = std::make_shared<Snapshot>();
  auto second = std::make_shared<Snapshot>();
  REQUIRE(gate.apply({1, 1, 1, first}));
  REQUIRE(gate.apply({1, 1, 2, {}}));
  CHECK_FALSE(gate.current());
  CHECK_FALSE(gate.apply({1, 1, 1, first}));
  REQUIRE(gate.apply({1, 2, 3, second}));
  CHECK(gate.current() == second);
  CHECK_FALSE(gate.apply({1, 1, 4, first}));
}

TEST_CASE("source generation requires valid tokens and serial revisions") {
  SourcePresentationGate gate;
  auto state = std::make_shared<Snapshot>();
  CHECK_FALSE(gate.apply({0, 1, 1, state}));
  CHECK_FALSE(gate.apply({1, 0, 1, state}));
  CHECK_FALSE(gate.apply({1, 1, 0, {}}));
  CHECK_FALSE(gate.apply({1, 0, 1, {}}));
  REQUIRE(gate.apply({1, 1, 1, state}));
  CHECK_FALSE(gate.apply({1, 1, 1, {}}));
}
