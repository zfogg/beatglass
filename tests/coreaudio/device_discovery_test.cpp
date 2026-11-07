#include "beatglass/capture/source.hpp"
#include <catch2/catch_test_macros.hpp>
#include <set>
#include <string>
#include <vector>

namespace beatglass::coreaudio {
std::vector<SourceInfo> enumerate_outputs(std::string& error);
}

TEST_CASE("Core Audio output discovery returns identifiable live sources", "[coreaudio]") {
  std::string error = "stale";
  const auto sources = beatglass::coreaudio::enumerate_outputs(error);
  REQUIRE(error.empty());
  std::set<std::string> ids;
  for (const auto& source : sources) {
    CHECK_FALSE(source.id.empty());
    CHECK_FALSE(source.name.empty());
    CHECK(source.live);
    CHECK_FALSE(source.seekable);
    CHECK(ids.insert(source.id).second);
  }
}
