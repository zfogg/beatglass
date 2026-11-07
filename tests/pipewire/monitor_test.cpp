#include <catch2/catch_test_macros.hpp>
#include "beatglass/capture/detail/pipewire_monitor.hpp"
#include <chrono>
#include <cstdlib>
#include <thread>
using namespace beatglass::pipewire;
TEST_CASE("monitor rejects missing selection and zero epoch") {
  Monitor invalid(0, {1}); CHECK_FALSE(invalid.start({"123", "sink", ""}).attempt);
  Monitor monitor(1, {1}); CHECK_FALSE(monitor.graph_clock()); CHECK_FALSE(monitor.start({"", "sink", ""}).attempt); CHECK_FALSE(monitor.start({"default", "sink", ""}).attempt); CHECK(monitor.state() == MonitorState::stopped);
}
TEST_CASE("unavailable PipeWire server never starts and stop quiesces") {
  REQUIRE(setenv("PIPEWIRE_REMOTE", "beatglass-test-nonexistent-server", 1) == 0);
  const auto enumeration = enumerate_monitor_sources(std::chrono::milliseconds(500));
  CHECK(enumeration.outcome == beatglass::EnumerationOutcome::error);
  CHECK(enumeration.error == beatglass::SourceError::backend_error);
  CHECK(enumeration.sources.empty());
  Monitor monitor(3, {1});
  const auto request = monitor.start({"123", "sink", ""});
  CHECK(request.error == beatglass::SourceError::backend_error);
  CHECK_FALSE(request.attempt);
  const bool accepted = request.attempt.has_value();
  if (accepted) {
    for (int n=0; n<100 && monitor.state() == MonitorState::starting; ++n)
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  CHECK(monitor.state() != MonitorState::streaming);
  monitor.stop();
  CHECK(monitor.state() == MonitorState::stopped);
  CHECK_FALSE(monitor.start({"123", "sink", ""}).attempt);
  unsetenv("PIPEWIRE_REMOTE");
}
