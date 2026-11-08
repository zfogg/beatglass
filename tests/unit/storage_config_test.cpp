#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/storage_config.hpp"
TEST_CASE("storage configuration requires an explicit database file") { std::string error; REQUIRE_FALSE(beatglass::library::valid_storage_config({}, &error)); REQUIRE_FALSE(error.empty()); REQUIRE(beatglass::library::valid_storage_config({"cache.sqlite"})); }
