#include <catch2/catch_test_macros.hpp>
#include "beatglass/capture/detail/pipewire_discovery.hpp"
#include <cstdlib>
#include <cerrno>

TEST_CASE("discovery classifies permission and daemon errors as retryable") {
  beatglass::pipewire::Discovery permission{{}, -EACCES, "permission denied"};
  beatglass::pipewire::Discovery daemon{{}, -ECONNREFUSED, "daemon unavailable"};
  beatglass::pipewire::Discovery malformed{{}, -EINVAL, "invalid request"};
  CHECK(permission.retryable());
  CHECK(daemon.retryable());
  CHECK_FALSE(malformed.retryable());
}

TEST_CASE("registry discovery selects only addressable output sinks") {
  beatglass::pipewire::RegistrySnapshot nodes;
  constexpr auto node = "PipeWire:Interface:Node";
  nodes.add(1, node, "Audio/Sink", "123", "output", "Speakers");
  nodes.add(2, node, "Audio/Source", "124", "microphone", "Mic");
  nodes.add(3, node, "Audio/Sink", "", "bad", nullptr);
  nodes.add(4, "PipeWire:Interface:Device", "Audio/Sink", "125", "device", nullptr);
  nodes.add(6, node, "Audio/Sink", "123", "output-duplicate", "Duplicate");
  auto sinks = nodes.sinks();
  REQUIRE(sinks.size() == 1);
  CHECK(sinks[0].serial == "123");
  CHECK(sinks[0].description == "Speakers");
}

TEST_CASE("registry discovery de-duplicates a sink serial across registry globals") {
  beatglass::pipewire::RegistrySnapshot nodes;
  nodes.add(1, "PipeWire:Interface:Node", "Audio/Sink", "same", "first", "First");
  nodes.add(2, "PipeWire:Interface:Node", "Audio/Sink", "same", "second", "Second");
  const auto sinks = nodes.sinks();
  REQUIRE(sinks.size() == 1);
  CHECK(sinks.front().serial == "same");
  CHECK(sinks.front().description == "Second");
}
TEST_CASE("registry snapshot handles fallback names and removal") {
  beatglass::pipewire::RegistrySnapshot nodes;
  nodes.add(5, "PipeWire:Interface:Node", "Audio/Sink", "5", "sink", nullptr);
  REQUIRE(nodes.sinks().size() == 1);
  CHECK(nodes.sinks()[0].description == "sink");
  nodes.remove(5);
  CHECK(nodes.sinks().empty());
}
TEST_CASE("unreachable PipeWire daemon is an error, not an empty success") {
  REQUIRE(setenv("PIPEWIRE_REMOTE", "beatglass-test-nonexistent-server", 1) == 0);
  auto result = beatglass::pipewire::discover_sinks(std::chrono::milliseconds(500));
  unsetenv("PIPEWIRE_REMOTE");
  CHECK_FALSE(result.error.empty());
  CHECK(result.error_code != 0);
  CHECK(result.sinks.empty());
}
