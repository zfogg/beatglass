#include "windows_default_tempo.hpp"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>
#include <stdexcept>

using namespace std::chrono_literals;
TEST_CASE("Default follower clears on selection loss and failed startup") {
  beatglass::SourceEpochAllocator epochs;
  beatglass::windows::EndpointResult result;
  beatglass::windows::DefaultTempoFollower follower(epochs, [&] { return result; });
  REQUIRE(follower.start());
  auto now = std::chrono::steady_clock::now();
  REQUIRE(follower.tick(now));
  REQUIRE(follower.status() == beatglass::windows::DefaultTempoFollower::Status::waiting);
  REQUIRE_FALSE(follower.takeUpdate());
  result.endpoints.push_back({"not-a-real-endpoint", "missing", true});
  REQUIRE(follower.tick(now + 2s));
  auto first = follower.takeUpdate();
  REQUIRE(first);
  REQUIRE_FALSE(first->snapshot);
  REQUIRE(first->revision == 1);
  for (int i = 0; i < 200 && !follower.endpoint().empty(); ++i) {
    REQUIRE(follower.tick(now + 2s + std::chrono::milliseconds(i * 10)));
    std::this_thread::sleep_for(10ms);
  }
  REQUIRE(follower.endpoint().empty());
  REQUIRE(follower.status() == beatglass::windows::DefaultTempoFollower::Status::error);
  auto failure = follower.takeUpdate();
  REQUIRE(failure);
  REQUIRE_FALSE(failure->snapshot);
  REQUIRE(failure->revision > first->revision);
  result.endpoints.clear();
  follower.stop();
  auto stopped = follower.takeUpdate();
  REQUIRE(stopped);
  REQUIRE(stopped->revision > failure->revision);
  REQUIRE(follower.tick(now + 10s));
  REQUIRE(follower.endpoint().empty());
  REQUIRE_FALSE(follower.takeUpdate());
}

TEST_CASE("Default follower fails closed on ambiguous discovery") {
  beatglass::SourceEpochAllocator epochs;
  beatglass::windows::EndpointResult result;
  result.endpoints = {{"first", "first", true}, {"second", "second", true}};
  beatglass::windows::DefaultTempoFollower follower(epochs, [&] { return result; });
  REQUIRE(follower.start());
  REQUIRE(follower.tick(std::chrono::steady_clock::now()));
  REQUIRE(follower.endpoint().empty());
  REQUIRE(follower.status() == beatglass::windows::DefaultTempoFollower::Status::error);
  REQUIRE_FALSE(follower.diagnostic().empty());
  REQUIRE_FALSE(follower.takeUpdate());
}

TEST_CASE("Discovery exceptions produce a clear and stop the follower") {
  beatglass::SourceEpochAllocator epochs;
  beatglass::windows::DefaultTempoFollower follower(epochs, []() -> beatglass::windows::EndpointResult { throw std::runtime_error("discovery failed"); });
  REQUIRE(follower.start());
  REQUIRE_FALSE(follower.tick(std::chrono::steady_clock::now()));
  REQUIRE(follower.endpoint().empty());
  auto update = follower.takeUpdate();
  REQUIRE(update);
  REQUIRE_FALSE(update->snapshot);
}

TEST_CASE("Default follower changes selected active endpoints without changing the OS default", "[.][native-default]") {
  auto result = beatglass::windows::enumerate_render_endpoints();
  REQUIRE(result.error.empty());
  if (result.endpoints.size() < 2) SKIP("At least two active render endpoints are required");
  for (auto& item : result.endpoints) item.is_default = false;
  result.endpoints[0].is_default = true;
  beatglass::SourceEpochAllocator epochs;
  beatglass::windows::DefaultTempoFollower follower(epochs, [&] { return result; });
  REQUIRE(follower.start());
  auto now = std::chrono::steady_clock::now();
  REQUIRE(follower.tick(now));
  REQUIRE(follower.endpoint() == result.endpoints[0].id);
  auto initial = follower.takeUpdate();
  REQUIRE(initial);
  REQUIRE_FALSE(initial->snapshot);
  for (int i = 0; i < 30; ++i) { REQUIRE(follower.tick(now + std::chrono::milliseconds(i * 10))); std::this_thread::sleep_for(10ms); }
  result.endpoints[0].is_default = false;
  result.endpoints[1].is_default = true;
  REQUIRE(follower.tick(now + 2s));
  REQUIRE(follower.endpoint() == result.endpoints[1].id);
  auto changed = follower.takeUpdate();
  REQUIRE(changed);
  REQUIRE_FALSE(changed->snapshot);
  REQUIRE(changed->revision > initial->revision);
  const auto recovered_id = result.endpoints[1].id;
  result.endpoints[1].id = "not-a-real-endpoint";
  REQUIRE(follower.tick(now + 4s));
  for (int i = 0; i < 200 && !follower.endpoint().empty(); ++i) {
    REQUIRE(follower.tick(now + 4s + std::chrono::milliseconds(i * 10)));
    std::this_thread::sleep_for(10ms);
  }
  REQUIRE(follower.endpoint().empty());
  REQUIRE(follower.status() == beatglass::windows::DefaultTempoFollower::Status::error);
  auto failed = follower.takeUpdate();
  REQUIRE(failed);
  REQUIRE_FALSE(failed->snapshot);
  REQUIRE(failed->revision > changed->revision);
  result.endpoints[1].id = recovered_id;
  REQUIRE(follower.tick(now + 8s));
  REQUIRE(follower.endpoint() == recovered_id);
  for (int i = 0; i < 200 && follower.status() == beatglass::windows::DefaultTempoFollower::Status::starting; ++i) {
    REQUIRE(follower.tick(now + 8s + std::chrono::milliseconds(i * 10)));
    std::this_thread::sleep_for(10ms);
  }
  REQUIRE(follower.status() == beatglass::windows::DefaultTempoFollower::Status::running);
  follower.stop();
}
