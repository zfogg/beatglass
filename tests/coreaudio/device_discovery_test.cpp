#include "beatglass/capture/coreaudio_internal.hpp"
#include <catch2/catch_test_macros.hpp>
#include <set>
#include <string>
#include <vector>


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


TEST_CASE("device list retries a change between size and read", "[coreaudio]") {
  int reads = 0;
  const auto query = [&](std::uint32_t& size, std::uint32_t* data) -> int {
    if (!data) { size = reads == 0 ? 4 : 8; return 0; }
    if (++reads == 1) return -1;
    data[0] = 10; data[1] = 20; size = 8; return 0;
  };
  std::vector<std::uint32_t> ids;
  int status = 0;
  REQUIRE(beatglass::coreaudio::detail::read_device_list(query, ids, status));
  CHECK((ids == std::vector<std::uint32_t>{10, 20}));
  CHECK(reads == 2);
}

TEST_CASE("device list accepts a successful empty result without stale IDs", "[coreaudio]") {
  std::vector<std::uint32_t> ids{42, 99};
  int status = -1;
  const auto empty = [](std::uint32_t& size, std::uint32_t* data) -> int {
    if (!data) size = 0;
    return 0;
  };

  REQUIRE(beatglass::coreaudio::detail::read_device_list(empty, ids, status));
  CHECK(ids.empty());
  CHECK(status == 0);
}

TEST_CASE("device list fails closed after repeated races or malformed size", "[coreaudio]") {
  std::vector<std::uint32_t> ids{99};
  int status = 0;
  int reads = 0;
  const auto changing = [&](std::uint32_t& size, std::uint32_t* data) -> int {
    size = 4;
    if (data) { ++reads; return -1; }
    return 0;
  };
  CHECK_FALSE(beatglass::coreaudio::detail::read_device_list(changing, ids, status));
  CHECK(ids.empty());
  CHECK(reads == 3);
  CHECK(status == -1);
  const auto malformed = [](std::uint32_t& size, std::uint32_t*) -> int { size = 3; return 0; };
  CHECK_FALSE(beatglass::coreaudio::detail::read_device_list(malformed, ids, status));
  CHECK(ids.empty());
}
