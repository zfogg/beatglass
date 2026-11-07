#include <catch2/catch_test_macros.hpp>
#include "../../src/desktop/collection_requests.hpp"
#include <string>
#include <vector>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

TEST_CASE("browse replacement rejects stale owned pages without affecting analysis") {
  CollectionRequests<std::vector<std::string>, std::string> requests;
  auto job = requests.beginAnalysis(); REQUIRE(job);
  auto old = requests.beginBrowse(); auto current = requests.beginBrowse();
  CHECK_FALSE(requests.finishBrowse(old, {"old"}));
  REQUIRE(requests.finishBrowse(current, {"new"}));
  REQUIRE(requests.page()); CHECK(requests.page()->at(0) == "new");
  CHECK_FALSE(requests.finishBrowse(current, {"duplicate"}));
  REQUIRE(requests.finishAnalysis(job, "committed"));
  REQUIRE(requests.terminal()); CHECK(*requests.terminal() == "committed");
}

TEST_CASE("analysis terminal remains until acknowledgement and stale jobs cannot replace it") {
  CollectionRequests<int, std::string> requests;
  auto first = requests.beginAnalysis(); REQUIRE(first);
  CHECK_FALSE(requests.beginAnalysis());
  requests.clearBrowse();
  REQUIRE(requests.finishAnalysis(first, "committed after cancellation request"));
  CHECK_FALSE(requests.beginAnalysis());
  CHECK_FALSE(requests.finishAnalysis(first, "cancelled"));
  requests.acknowledgeAnalysis();
  auto second = requests.beginAnalysis(); REQUIRE(second);
  CHECK_FALSE(requests.finishAnalysis(first, "stale"));
  REQUIRE(requests.finishAnalysis(second, "cancelled"));
  CHECK(*requests.terminal() == "cancelled");
  auto owned = requests.takeAnalysis(); REQUIRE(owned); CHECK(*owned == "cancelled");
  CHECK_FALSE(requests.terminal()); CHECK(requests.beginAnalysis());
}

TEST_CASE("invalid browse replacement requests cancellation of blocked prior work") {
  using namespace std::chrono_literals;
  CollectionRequests<int, std::string> requests;
  auto analysis = requests.beginAnalysis(); REQUIRE(analysis);
  auto previous = requests.beginBrowse();
  std::promise<void> started, exited; auto done = exited.get_future();
  std::mutex mutex; std::condition_variable_any waiting;
  std::jthread worker([&](std::stop_token stop) {
    std::unique_lock lock(mutex); started.set_value(); waiting.wait(lock, stop, [] { return false; }); exited.set_value();
  });
  started.get_future().wait();
  auto replacement = requests.beginBrowse([&] { worker.request_stop(); });
  // The replacement is invalid and is never submitted to a backend.
  REQUIRE(replacement); REQUIRE(done.wait_for(5s) == std::future_status::ready);
  CHECK_FALSE(requests.finishBrowse(previous, 1));
  REQUIRE(requests.finishAnalysis(analysis, "completed"));
  CHECK(*requests.terminal() == "completed");
}
