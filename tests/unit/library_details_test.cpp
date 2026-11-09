#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/details.hpp"
#include <fstream>
#include <chrono>
TEST_CASE("library details inspect supported audio without modifying it") { auto p=std::filesystem::temp_directory_path()/("beatglass-details-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".MP3"); {std::ofstream(p)<<"audio"; auto d=beatglass::library::inspect_file(p); REQUIRE(d); CHECK(d->name==p.filename().string()); CHECK(d->extension==".mp3"); CHECK(d->size_bytes==5);} std::filesystem::remove(p); }
TEST_CASE("library details reject unsupported and missing paths") { auto p=std::filesystem::temp_directory_path()/"beatglass-details.txt"; std::ofstream(p)<<"x"; CHECK_FALSE(beatglass::library::inspect_file(p)); std::filesystem::remove(p); CHECK_FALSE(beatglass::library::inspect_file(p)); }
TEST_CASE("library details marks metadata-only M4A without claiming analysis support") { const auto p=std::filesystem::temp_directory_path()/"beatglass-details.m4a"; { std::ofstream(p)<<"metadata-only fixture"; const auto details=beatglass::library::inspect_file(p); REQUIRE(details); CHECK(details->extension==".m4a"); CHECK_FALSE(details->analysis_supported); } std::filesystem::remove(p); }
