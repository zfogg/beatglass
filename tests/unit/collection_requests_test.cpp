#include <catch2/catch_test_macros.hpp>
#include "../../src/desktop/collection_requests.hpp"
#include <string>
#include <vector>

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
}
