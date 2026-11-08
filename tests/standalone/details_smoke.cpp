#include "beatglass/library/details.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {
struct TempDir {
  std::filesystem::path path = std::filesystem::temp_directory_path() /
      ("beatglass-details-smoke-" + std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()));
  TempDir() { std::filesystem::create_directory(path); }
  ~TempDir() { std::error_code ec; std::filesystem::remove_all(path, ec); }
};

void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "details smoke failure: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}
}  // namespace

int main() {
  TempDir dir;
  const auto audio = dir.path / "Sample.MP3";
  std::ofstream(audio) << "audio";
  const auto details = beatglass::library::inspect_file(audio);
  require(details.has_value(), "supported regular audio was rejected");
  require(details->path == audio, "caller path spelling was not retained");
  require(details->name == "Sample.MP3", "filename was not retained");
  require(details->extension == ".mp3", "extension was not normalized");
  require(details->size_bytes == 5, "file size was not reported");

  const auto unsupported = dir.path / "notes.txt";
  std::ofstream(unsupported) << "text";
  require(!beatglass::library::inspect_file(unsupported),
          "unsupported extension was accepted");
  require(!beatglass::library::inspect_file(dir.path / "missing.MP3"),
          "missing file was accepted");
  return EXIT_SUCCESS;
}
