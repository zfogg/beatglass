#include "beatglass/library/catalog.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {
struct TempDir {
  std::filesystem::path path = std::filesystem::temp_directory_path() /
      ("beatglass-catalog-smoke-" + std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()));
  TempDir() { std::filesystem::create_directory(path); }
  ~TempDir() { std::error_code ec; std::filesystem::remove_all(path, ec); }
  void file(const char* name) { std::ofstream(path / name) << "audio"; }
};

void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "catalog smoke failure: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}
}  // namespace

int main() {
  TempDir dir;
  dir.file("zeta.WAV");
  dir.file("Alpha.mp3");
  dir.file("notes.txt");
  dir.file("stream.ogg");
  std::filesystem::create_directory(dir.path / "Mixes");

  const auto result = beatglass::library::browse(dir.path);
  require(result.diagnostic.empty(), "unexpected browse diagnostic");
  require(result.entries.size() == 3, "unsupported files were advertised");
  require(result.entries[0].name == "Alpha.mp3", "case-folded ordering changed");
  require(result.entries[1].name == "Mixes", "directory navigation entry missing");
  require(result.entries[2].name == "zeta.WAV", "supported audio entry missing");
  require(result.entries[0].title == "Alpha" && result.entries[0].format == "MP3",
          "audio display metadata was not derived from the filename");
  require(result.entries[0].bytes == 5 && result.entries[0].duration_seconds < 0.0,
          "filesystem-only catalog metadata contract changed");
  require(result.entries[1].directory && result.entries[1].bytes == 0,
          "directory entry metadata was not normalized");

  const auto filtered = beatglass::library::browse(dir.path, "SUM");
  require(filtered.entries.empty(), "filter returned an unexpected entry");
  const auto missing = beatglass::library::browse(dir.path / "missing");
  require(missing.entries.empty() && !missing.diagnostic.empty(),
          "missing directory was not reported atomically");
  std::filesystem::remove(dir.path / "Alpha.mp3");
  const auto after_remove = beatglass::library::browse(dir.path, "alpha");
  require(after_remove.entries.empty() && after_remove.diagnostic.empty(),
          "removed catalog entry remained visible");
  std::filesystem::remove_all(dir.path / "Mixes");
  const auto after_directory_remove = beatglass::library::browse(dir.path, "mix");
  require(after_directory_remove.entries.empty() && after_directory_remove.diagnostic.empty(),
          "removed navigation directory remained visible");
  return EXIT_SUCCESS;
}
