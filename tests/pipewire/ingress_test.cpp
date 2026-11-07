#include <catch2/catch_test_macros.hpp>
#include "beatglass/capture/detail/pipewire_ingress.hpp"
#include <array>
using namespace beatglass::pipewire;
TEST_CASE("monitor ingress copies frames with explicit epoch and discontinuity") {
  MonitorIngress ingress(42);
  const std::array<float, 4> pcm{0.25f, -0.5f, 0.75f, 1.0f};
  auto bytes = std::as_bytes(std::span{pcm});
  REQUIRE(ingress.ingest(bytes, 0, bytes.size(), 8, false, true) == IngressResult::copied);
  MonitorIngress::Block block;
  REQUIRE(ingress.pop(block));
  CHECK(block.epoch == 42); CHECK(block.first_frame == 0); CHECK(block.discontinuity);
  CHECK(block.format.sample_rate == 48000); CHECK(block.sample_count == 4);
  CHECK(block.samples[1] == -0.5f);
}
TEST_CASE("monitor ingress overflow advances the timeline and marks the next block") {
  MonitorIngress ingress(1);
  const std::array<float, 2> pcm{1, -1};
  auto bytes = std::as_bytes(std::span{pcm});
  for (int i=0; i<31; ++i) REQUIRE(ingress.ingest(bytes, 0, bytes.size(), 8, false, false) == IngressResult::copied);
  REQUIRE(ingress.ingest(bytes, 0, bytes.size(), 8, false, false) == IngressResult::dropped);
  CHECK(ingress.dropped_frames() == 1);
  MonitorIngress::Block block;
  REQUIRE(ingress.pop(block));
  REQUIRE(ingress.ingest(bytes, 0, bytes.size(), 8, false, false) == IngressResult::copied);
  for (int i=0; i<31; ++i) REQUIRE(ingress.pop(block));
  CHECK(block.first_frame == 32); CHECK(block.discontinuity);
}
TEST_CASE("malformed or corrupted input permanently fails the epoch") {
  const std::array<float, 2> pcm{1, -1};
  auto bytes = std::as_bytes(std::span{pcm});
  SECTION("bounds") { MonitorIngress ingress(1); CHECK(ingress.ingest(bytes, 1, bytes.size(), 8, false, false) == IngressResult::invalid); CHECK(ingress.ingest(bytes, 0, bytes.size(), 8, false, false) == IngressResult::invalid); }
  SECTION("corruption") { MonitorIngress ingress(1); CHECK(ingress.ingest(bytes, 0, bytes.size(), 8, true, false) == IngressResult::invalid); }
  SECTION("epoch") { MonitorIngress ingress(0); CHECK(ingress.ingest(bytes, 0, bytes.size(), 8, false, false) == IngressResult::invalid); }
}
