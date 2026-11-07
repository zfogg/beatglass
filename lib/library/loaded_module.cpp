#include "beatglass/library/detail/loaded_module.hpp"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#elif defined(__APPLE__) || defined(__linux__)
#include <dlfcn.h>
#endif
namespace beatglass::library_detail {
std::optional<std::filesystem::path> loaded_module_path(const void* address) {
  if (!address) return std::nullopt;
#ifdef _WIN32
  HMODULE module{};
  if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(address), &module)) return std::nullopt;
  struct Release { HMODULE module; ~Release() { FreeLibrary(module); } } release{module};
  std::wstring buffer(32768, L'\0');
  const DWORD count=GetModuleFileNameW(module,buffer.data(),static_cast<DWORD>(buffer.size()));
  if (!count || count>=buffer.size()) return std::nullopt;
  buffer.resize(count); return std::filesystem::path(buffer);
#elif defined(__APPLE__) || defined(__linux__)
  Dl_info info{};
  if (!dladdr(address,&info) || !info.dli_fname || !*info.dli_fname) return std::nullopt;
  return std::filesystem::path(info.dli_fname);
#else
  return std::nullopt;
#endif
}
}
