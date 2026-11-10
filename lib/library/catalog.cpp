#include "beatglass/library/catalog.hpp"

#include <algorithm>
#include <cctype>
#include <system_error>

namespace beatglass::library {
namespace {
std::string folded(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}
}

std::string audio_format_for_path(const std::filesystem::path& path) {
  const auto extension = folded(path.extension().string());
  if (extension == ".wav") return "WAV";
  if (extension == ".flac") return "FLAC";
  if (extension == ".mp3") return "MP3";
  if (extension == ".aiff" || extension == ".aif") return "AIFF";
  return {};
}

namespace {
bool supported_audio(const std::filesystem::path& path) { return !audio_format_for_path(path).empty(); }
}

CatalogResult browse(const std::filesystem::path& folder, std::string search) {
  CatalogResult result;
  std::error_code error;
  if (!std::filesystem::is_directory(folder, error)) {
    result.diagnostic = error ? error.message() : "not a directory";
    return result;
  }
  const auto query = folded(std::move(search));
  for (std::filesystem::directory_iterator it(folder, error), end; it != end && !error; it.increment(error)) {
    const auto& entry = *it;
    // Library browsing is intentionally one level deep; do not follow links
    // into unrelated trees or expose a linked source under a misleading path.
    if (entry.is_symlink(error)) {
      if (error) break;
      continue;
    }
    const bool directory = entry.is_directory(error);
    if (error) break;
    if (!directory && (!entry.is_regular_file(error) || !supported_audio(entry.path()))) {
      if (error) break;
      continue;
    }
    const auto name = entry.path().filename().string();
    if (!query.empty() && folded(name).find(query) == std::string::npos) continue;
    std::uintmax_t bytes = 0;
    if (!directory) {
      std::error_code size_error;
      bytes = entry.file_size(size_error);
      if (size_error) bytes = 0;
    }
    std::string format;
    if (!directory) {
      format = audio_format_for_path(entry.path());
    }
    const auto title = entry.path().stem().string();
    // Duration requires opening and decoding the source. Catalog browsing is
    // intentionally filesystem-only, so it reports an explicit unknown value
    // until the analysis/metadata worker supplies decoded duration.
    result.entries.push_back({entry.path(), name, title, {}, directory, bytes, std::move(format), -1.0});
  }
  if (error) { result.entries.clear(); result.diagnostic = error.message(); return result; }
  std::sort(result.entries.begin(), result.entries.end(), [](const auto& a, const auto& b) {
    const auto af = folded(a.name), bf = folded(b.name);
    if (af != bf) return af < bf;
    if (a.name != b.name) return a.name < b.name;
    return a.path.string() < b.path.string();
  });
  return result;
}

CatalogResult browse_recursive(const std::filesystem::path& folder, std::string search) {
  CatalogResult result;
  std::error_code error;
  if (!std::filesystem::is_directory(folder, error)) {
    result.diagnostic = error ? error.message() : "not a directory";
    return result;
  }
  const auto query = folded(std::move(search));
  std::filesystem::recursive_directory_iterator it(
      folder, std::filesystem::directory_options::skip_permission_denied, error);
  const std::filesystem::recursive_directory_iterator end;
  for (; it != end && !error; it.increment(error)) {
    const auto& entry = *it;
    if (entry.is_symlink(error)) { if (error) break; it.disable_recursion_pending(); continue; }
    const bool directory = entry.is_directory(error);
    if (error) break;
    if (!directory && (!entry.is_regular_file(error) || !supported_audio(entry.path()))) {
      if (error) break;
      continue;
    }
    const auto name = entry.path().filename().string();
    if (!query.empty() && folded(name).find(query) == std::string::npos) continue;
    std::uintmax_t bytes = 0;
    if (!directory) { std::error_code size_error; bytes = entry.file_size(size_error); if (size_error) bytes = 0; }
    std::string format;
    if (!directory) {
      format = audio_format_for_path(entry.path());
    }
    result.entries.push_back({entry.path(), name, entry.path().stem().string(), {}, directory,
                              bytes, std::move(format), -1.0});
  }
  if (error) { result.entries.clear(); result.diagnostic = error.message(); return result; }
  std::sort(result.entries.begin(), result.entries.end(), [](const auto& a, const auto& b) {
    const auto af = folded(a.path.string()), bf = folded(b.path.string());
    if (af != bf) return af < bf;
    return a.path.string() < b.path.string();
  });
  return result;
}
} // namespace beatglass::library
