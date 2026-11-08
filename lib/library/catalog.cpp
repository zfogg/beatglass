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
bool supported_audio(const std::filesystem::path& path) {
  const auto extension = folded(path.extension().string());
  return extension == ".wav" || extension == ".flac" || extension == ".mp3" ||
         extension == ".ogg" || extension == ".aiff" || extension == ".aif" ||
         extension == ".m4a";
}
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
    const bool directory = entry.is_directory(error);
    if (error) break;
    if (!directory && (!entry.is_regular_file(error) || !supported_audio(entry.path()))) {
      if (error) break;
      continue;
    }
    const auto name = entry.path().filename().string();
    if (!query.empty() && folded(name).find(query) == std::string::npos) continue;
    result.entries.push_back({entry.path(), name, directory});
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
} // namespace beatglass::library
