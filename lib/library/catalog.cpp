#include "beatglass/library/catalog.hpp"

#include <algorithm>
#include <cctype>
#include <system_error>
#include <deque>
#include <unordered_set>

namespace beatglass::library {
namespace {
std::filesystem::path relative_key(const std::filesystem::path& path,
                                   const std::filesystem::path& root,
                                   std::error_code& error) {
  return std::filesystem::relative(path, root, error);
}
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

CatalogResult browse(const std::filesystem::path& folder, std::string search,
                     std::size_t max_entries, std::stop_token stop) {
  CatalogResult result;
  if (stop.stop_requested()) { result.cancelled = true; result.diagnostic = "catalog query cancelled"; return result; }
  std::error_code error;
  if (!std::filesystem::is_directory(folder, error)) {
    result.status = error ? CatalogStatus::filesystem_error : CatalogStatus::invalid_root;
    result.diagnostic = error ? error.message() : "not a directory";
    return result;
  }
  result.query_root = std::filesystem::absolute(folder, error);
  if (error) { result.status = CatalogStatus::filesystem_error; result.diagnostic = error.message(); return result; }
  const auto query = folded(std::move(search));
  for (std::filesystem::directory_iterator it(folder, error), end; it != end && !error; it.increment(error)) {
    if (stop.stop_requested()) { result.entries.clear(); result.cancelled = true; result.diagnostic = "catalog query cancelled"; return result; }
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
    std::error_code relative_error;
    const auto key = relative_key(entry.path(), folder, relative_error);
    if (relative_error) { result.entries.clear(); result.status = CatalogStatus::filesystem_error; result.diagnostic = relative_error.message(); return result; }
    result.entries.push_back({entry.path(), key, name, title, {}, directory, bytes, std::move(format), -1.0});
  }
  if (error) { result.entries.clear(); result.status = CatalogStatus::filesystem_error; result.diagnostic = error.message(); return result; }
  std::sort(result.entries.begin(), result.entries.end(), [](const auto& a, const auto& b) {
    const auto af = folded(a.name), bf = folded(b.name);
    if (af != bf) return af < bf;
    if (a.name != b.name) return a.name < b.name;
    return a.path.string() < b.path.string();
  });
  if (result.entries.size() > max_entries) {
    result.entries.clear();
    result.status = CatalogStatus::entry_limit_exceeded;
    result.diagnostic = "catalog entry limit exceeded";
  }
  return result;
}

CatalogResult browse_recursive(const std::filesystem::path& folder, std::string search,
                               std::size_t max_entries, std::stop_token stop) {
  CatalogResult result;
  if (stop.stop_requested()) { result.cancelled = true; result.diagnostic = "catalog query cancelled"; return result; }
  std::error_code error;
  if (!std::filesystem::is_directory(folder, error)) {
    result.status = error ? CatalogStatus::filesystem_error : CatalogStatus::invalid_root;
    result.diagnostic = error ? error.message() : "not a directory";
    return result;
  }
  result.query_root = std::filesystem::absolute(folder, error);
  if (error) { result.status = CatalogStatus::filesystem_error; result.diagnostic = error.message(); return result; }
  const auto query = folded(std::move(search));
  std::filesystem::recursive_directory_iterator it(
      folder, std::filesystem::directory_options::skip_permission_denied, error);
  const std::filesystem::recursive_directory_iterator end;
  for (; it != end && !error; it.increment(error)) {
    if (stop.stop_requested()) { result.entries.clear(); result.cancelled = true; result.diagnostic = "catalog query cancelled"; return result; }
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
    std::error_code relative_error;
    const auto key = relative_key(entry.path(), folder, relative_error);
    if (relative_error) { result.entries.clear(); result.status = CatalogStatus::filesystem_error; result.diagnostic = relative_error.message(); return result; }
    result.entries.push_back({entry.path(), key, name, entry.path().stem().string(), {}, directory,
                              bytes, std::move(format), -1.0});
  }
  if (error) { result.entries.clear(); result.status = CatalogStatus::filesystem_error; result.diagnostic = error.message(); return result; }
  std::sort(result.entries.begin(), result.entries.end(), [](const auto& a, const auto& b) {
    const auto af = folded(a.relative_path.generic_string()), bf = folded(b.relative_path.generic_string());
    if (af != bf) return af < bf;
    return a.relative_path.generic_string() < b.relative_path.generic_string();
  });
  if (result.entries.size() > max_entries) {
    result.entries.clear();
    result.status = CatalogStatus::entry_limit_exceeded;
    result.diagnostic = "catalog entry limit exceeded";
  }
  return result;
}

CatalogScanResult scan_roots(const std::vector<std::filesystem::path>& roots,
                             CatalogScanOptions options, std::stop_token stop) {
  CatalogScanResult result;
  const auto query = folded(std::move(options.search));
  std::unordered_set<std::string> seen;
  CatalogScanProgress progress{0, roots.size(), 0};
  auto report = [&] { if (options.progress) options.progress(progress); };
  if (options.max_pending_directories == 0) options.max_pending_directories = 1;
  for (const auto& configured : roots) {
    if (stop.stop_requested()) { result.entries.clear(); result.cancelled = true; result.diagnostic = "catalog scan cancelled"; return result; }
    CatalogRootResult root_result;
    std::error_code ec;
    root_result.root = std::filesystem::absolute(configured, ec);
    const bool directory = !ec && std::filesystem::is_directory(root_result.root, ec);
    if (ec || !directory) {
      // A configured path that is absent or not a directory is an invalid
      // root, even though the platform may report ENOENT/ERROR_PATH_NOT_FOUND
      // through the filesystem error code.
      root_result.status = CatalogStatus::invalid_root;
      root_result.diagnostic = directory ? "not a directory" : (ec ? ec.message() : "not a directory");
      result.roots.push_back(std::move(root_result));
      ++progress.roots_completed; report(); continue;
    }
    std::deque<std::filesystem::path> pending{root_result.root};
    const auto root_entry_start = result.entries.size();
    bool failed = false;
    while (!pending.empty() && !failed) {
      if (stop.stop_requested()) { result.entries.clear(); result.cancelled = true; result.diagnostic = "catalog scan cancelled"; return result; }
      const auto directory = std::move(pending.front()); pending.pop_front();
      for (std::filesystem::directory_iterator it(directory, std::filesystem::directory_options::skip_permission_denied, ec), end;
           it != end && !ec; it.increment(ec)) {
        if (stop.stop_requested()) { result.entries.clear(); result.cancelled = true; result.diagnostic = "catalog scan cancelled"; return result; }
        const auto& entry = *it;
        if (entry.is_symlink(ec)) { if (ec) break; continue; }
        const bool is_dir = entry.is_directory(ec);
        if (ec) break;
        if (is_dir) {
          if (pending.size() >= options.max_pending_directories) { root_result.status = CatalogStatus::entry_limit_exceeded; root_result.diagnostic = "catalog pending directory limit exceeded"; failed = true; break; }
          pending.push_back(entry.path());
        }
        if (!is_dir && (!entry.is_regular_file(ec) || audio_format_for_path(entry.path()).empty())) { if (ec) break; continue; }
        const auto name = entry.path().filename().string();
        if (!query.empty() && folded(name).find(query) == std::string::npos) continue;
        std::error_code abs_ec;
        const auto absolute = std::filesystem::absolute(entry.path(), abs_ec);
        if (abs_ec) { root_result.status = CatalogStatus::filesystem_error; root_result.diagnostic = abs_ec.message(); failed = true; break; }
        const auto identity = absolute.lexically_normal().generic_string();
        if (!seen.insert(identity).second) continue;
        std::error_code rel_ec;
        const auto relative = std::filesystem::relative(entry.path(), root_result.root, rel_ec);
        if (rel_ec) { root_result.status = CatalogStatus::filesystem_error; root_result.diagnostic = rel_ec.message(); failed = true; break; }
        std::uintmax_t bytes = 0;
        if (!is_dir) { std::error_code size_ec; bytes = entry.file_size(size_ec); }
        result.entries.push_back({absolute, relative, name, entry.path().stem().string(), {}, is_dir, bytes,
                                  is_dir ? std::string{} : audio_format_for_path(entry.path()), -1.0});
        ++root_result.entries_seen; ++progress.entries_seen; report();
        if (result.entries.size() > options.max_entries) { result.entries.clear(); result.diagnostic = "catalog entry limit exceeded"; return result; }
      }
      if (ec) { root_result.status = CatalogStatus::filesystem_error; root_result.diagnostic = ec.message(); failed = true; }
    }
    if (failed) {
      result.entries.erase(result.entries.begin() + static_cast<std::ptrdiff_t>(root_entry_start), result.entries.end());
      if (result.diagnostic.empty()) result.diagnostic = root_result.diagnostic;
    }
    result.roots.push_back(std::move(root_result)); ++progress.roots_completed; report();
  }
  std::sort(result.entries.begin(), result.entries.end(), [](const auto& a, const auto& b) { return a.path.generic_string() < b.path.generic_string(); });
  return result;
}
} // namespace beatglass::library
