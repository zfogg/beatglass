#include "windows_endpoints.hpp"
#include <catch2/catch_test_macros.hpp>
#include <windows.h>
#include <set>

TEST_CASE("Render endpoint discovery returns owned unique identities") {
  const auto result = beatglass::windows::enumerate_render_endpoints();
  REQUIRE(result.error.empty());
  std::set<std::string> ids;
  unsigned defaults = 0;
  for (const auto& endpoint : result.endpoints) {
    REQUIRE_FALSE(endpoint.id.empty());
    REQUIRE_FALSE(endpoint.name.empty());
    REQUIRE(ids.insert(endpoint.id).second);
    defaults += endpoint.is_default;
  }
  REQUIRE(defaults <= 1);
}

TEST_CASE("Discovery works in an existing STA without uninitializing it") {
  const auto initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  REQUIRE(SUCCEEDED(initialized));
  const auto result = beatglass::windows::enumerate_render_endpoints();
  REQUIRE(result.error.empty());
  // A second STA initialization succeeds only if discovery did not tear down COM.
  const auto again = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  REQUIRE(again == S_FALSE);
  CoUninitialize();
  CoUninitialize();
}
