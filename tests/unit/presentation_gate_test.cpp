#include <catch2/catch_test_macros.hpp>
#include "beatglass/core/presentation_gate.hpp"
#include <memory>
using namespace beatglass;
TEST_CASE("presentation filters delayed updates after invalidation and restart") {
  PresentationGate gate; auto old = std::make_shared<Snapshot>(); old->observed_through = {1, 10};
  auto fresh = std::make_shared<Snapshot>(); fresh->observed_through = {2, 3};
  REQUIRE(gate.apply({0, old})); auto retained = gate.current(); REQUIRE(retained);
  REQUIRE(gate.apply({2, {}})); CHECK_FALSE(gate.current());
  CHECK_FALSE(gate.apply({1, old})); CHECK_FALSE(gate.current());
  REQUIRE(gate.apply({4, fresh})); CHECK(gate.current()->observed_through.epoch == 2);
  CHECK_FALSE(gate.apply({3, {}})); CHECK_FALSE(gate.apply({4, old}));
  CHECK(gate.current()->observed_through.epoch == 2); CHECK(retained->observed_through.epoch == 1);
}
