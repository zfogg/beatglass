#include "beatglass/core/visualization_presentation.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace beatglass;

TEST_CASE("visualization presentation accepts only current newer revisions") {
  VisualizationPresentation presentation;
  VisualizationFrame frame;
  frame.epoch = 4; frame.sample_rate = 48000; frame.first_frame = 1024; frame.end_frame = 2048;
  const auto generation = presentation.generation();
  REQUIRE(presentation.accept(generation, 1, frame));
  REQUIRE(presentation.current());
  CHECK(presentation.current()->first_frame == 1024);
  CHECK_FALSE(presentation.accept(generation, 1, frame));
  CHECK_FALSE(presentation.accept(generation - 1, 2, frame));
}

TEST_CASE("visualization presentation clear removes stale painted frame") {
  VisualizationPresentation presentation;
  VisualizationFrame frame;
  frame.epoch = 1; frame.sample_rate = 48000; frame.end_frame = 1;
  const auto old_generation = presentation.generation();
  REQUIRE(presentation.accept(old_generation, 1, frame));
  presentation.clear();
  CHECK_FALSE(presentation.current());
  CHECK(presentation.generation() > old_generation);
  CHECK_FALSE(presentation.accept(old_generation, 2, frame));
  REQUIRE(presentation.accept(presentation.generation(), 1, frame));
}

TEST_CASE("visualization presentation exposes the accepted revision for playback handoff") {
  VisualizationPresentation presentation;
  VisualizationFrame frame;
  frame.epoch = 7; frame.sample_rate = 48000; frame.end_frame = 1024;
  const auto generation = presentation.generation();
  CHECK(presentation.acceptedRevision() == 0);
  REQUIRE(presentation.accept(generation, 4, frame));
  CHECK(presentation.acceptedRevision() == 4);
  presentation.clear();
  CHECK(presentation.acceptedRevision() == 0);
  CHECK(presentation.generation() > generation);
}

TEST_CASE("visualization presentation rejects malformed frames before publication") {
  VisualizationPresentation presentation;
  VisualizationFrame frame;
  const auto generation = presentation.generation();
  CHECK_FALSE(presentation.accept(generation, 1, frame));
  frame.epoch = 1; frame.sample_rate = 48000; frame.end_frame = 1024;
  frame.magnitude[3] = -0.1f;
  CHECK_FALSE(presentation.accept(generation, 1, frame));
  frame.magnitude[3] = 0.1f;
  REQUIRE(presentation.accept(generation, 1, frame));
}
