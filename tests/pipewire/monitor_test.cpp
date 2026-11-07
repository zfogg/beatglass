#include <catch2/catch_test_macros.hpp>
#include "beatglass/capture/detail/pipewire_monitor.hpp"
#include <chrono>
#include <cstdlib>
#include <thread>
using namespace beatglass::pipewire;
TEST_CASE("monitor rejects missing selection and zero epoch") {
  Monitor invalid(0); CHECK_FALSE(invalid.start("123"));
  Monitor monitor(1); CHECK_FALSE(monitor.start("")); CHECK_FALSE(monitor.start("default")); CHECK(monitor.state() == MonitorState::stopped);
}
TEST_CASE("unavailable PipeWire server never starts and stop quiesces") {
  REQUIRE(setenv("PIPEWIRE_REMOTE", "beatglass-test-nonexistent-server", 1) == 0);
  Monitor monitor(3);
  const bool accepted = monitor.start("123");
  if (accepted) {
    for (int n=0; n<100 && monitor.state() == MonitorState::starting; ++n)
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  CHECK(monitor.state() != MonitorState::streaming);
  monitor.stop();
  CHECK(monitor.state() == MonitorState::stopped);
  CHECK_FALSE(monitor.start("123"));
  unsetenv("PIPEWIRE_REMOTE");
}
