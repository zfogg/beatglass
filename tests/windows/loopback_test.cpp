#include "windows_endpoints.hpp"
#include "windows_loopback.hpp"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;
TEST_CASE("Invalid endpoint interrupts and can be stopped and retried") {
  beatglass::windows::Loopback capture;
  REQUIRE_FALSE(capture.start("", 1));
  REQUIRE(capture.start("not-a-real-endpoint", 1));
  for (int i = 0; i < 200 && capture.state() == beatglass::windows::Loopback::State::starting; ++i) std::this_thread::sleep_for(10ms);
  REQUIRE(capture.state() == beatglass::windows::Loopback::State::interrupted);
  REQUIRE(capture.error_code() != 0);
  beatglass::windows::LoopbackPacket packet;
  REQUIRE_FALSE(capture.pop(packet));
  capture.stop();
  REQUIRE(capture.state() == beatglass::windows::Loopback::State::stopped);
  REQUIRE(capture.start("not-a-real-endpoint", 2));
  capture.stop();
}

TEST_CASE("Selected active render endpoint starts and stops loopback when available") {
  const auto list = beatglass::windows::enumerate_render_endpoints();
  REQUIRE(list.error.empty());
  if (list.endpoints.empty()) SKIP("No active render endpoint on this machine");
  beatglass::windows::Loopback capture;
  REQUIRE(capture.start(list.endpoints.front().id, 42));
  for (int i = 0; i < 300 && capture.state() == beatglass::windows::Loopback::State::starting; ++i) std::this_thread::sleep_for(10ms);
  INFO("WASAPI HRESULT: " << capture.error_code());
  REQUIRE(capture.state() == beatglass::windows::Loopback::State::running);
  REQUIRE_FALSE(capture.start(list.endpoints.front().id, 43));
  capture.stop();
  REQUIRE(capture.state() == beatglass::windows::Loopback::State::stopped);
  beatglass::windows::LoopbackPacket packet;
  REQUIRE_FALSE(capture.pop(packet));
}
