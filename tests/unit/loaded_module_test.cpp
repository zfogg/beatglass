#include "beatglass/library/detail/loaded_module.hpp"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
using namespace beatglass::library_detail;
namespace { int module_marker; }
TEST_CASE("loaded module observation locates the containing image but grants no cache identity") {
  CHECK_FALSE(loaded_module_path(nullptr));
#if defined(_WIN32) || defined(__APPLE__) || defined(__linux__)
  const auto observed=loaded_module_path(&module_marker);
  REQUIRE(observed); CHECK_FALSE(observed->empty()); CHECK(std::filesystem::exists(*observed));
#else
  CHECK_FALSE(loaded_module_path(&module_marker));
#endif
}

#ifdef _WIN32
TEST_CASE("Windows module observation resolves an address in a loaded DLL") {
  auto module=GetModuleHandleW(L"kernel32.dll"); REQUIRE(module);
  auto symbol=GetProcAddress(module,"GetCurrentProcessId"); REQUIRE(symbol);
  auto path=loaded_module_path(reinterpret_cast<const void*>(symbol)); REQUIRE(path);
  CHECK(std::filesystem::exists(*path));
}
#endif
