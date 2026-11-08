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
  dir.file("ambient.FLAC");
  dir.file("live.AIFF");
  dir.file("take.aif");
  std::ofstream(dir.path / "empty.wav", std::ios::binary);
  dir.file("podcast.m4a");
  dir.file("voice.AAC");
  dir.file("notes.txt");
  dir.file("stream.ogg");
  std::filesystem::create_directory(dir.path / "Mixes");
  std::filesystem::create_directory(dir.path / "Folder.WAV");

  const auto result = beatglass::library::browse(dir.path);
  require(result.diagnostic.empty(), "unexpected browse diagnostic");
  require(result.entries.size() == 8, "supported files were not advertised");
  require(result.entries[0].name == "Alpha.mp3", "case-folded ordering changed");
  require(result.entries[5].name == "Mixes", "directory navigation entry missing");
  require(result.entries[7].name == "zeta.WAV", "supported audio entry missing");
  require(result.entries[0].title == "Alpha" && result.entries[0].format == "MP3",
          "audio display metadata was not derived from the filename");
  require(result.entries[0].bytes == 5 && result.entries[0].duration_seconds < 0.0,
          "filesystem-only catalog metadata contract changed");
  require(result.entries[5].directory && result.entries[5].bytes == 0,
          "directory entry metadata was not normalized");
  const auto extension_named_directory = beatglass::library::browse(dir.path, "folder");
  require(extension_named_directory.entries.size() == 1 && extension_named_directory.entries[0].directory,
          "directory with an audio-like extension was filtered as a file");
  std::ofstream(dir.path / "Take One.mp3") << "audio";
  const auto spaced_source = beatglass::library::browse(dir.path, "TAKE ONE");
  require(spaced_source.entries.size() == 1 && spaced_source.entries[0].name == "Take One.mp3" &&
              spaced_source.entries[0].title == "Take One" && spaced_source.entries[0].path.filename() == "Take One.mp3",
          "source identity with spaces was not preserved");
  std::filesystem::remove(dir.path / "Take One.mp3");

  const auto filtered = beatglass::library::browse(dir.path, "SUM");
  require(filtered.entries.empty(), "filter returned an unexpected entry");
  const auto extension_filtered = beatglass::library::browse(dir.path, "MP3");
  require(extension_filtered.entries.size() == 1 && extension_filtered.entries[0].name == "Alpha.mp3",
          "case-insensitive catalog filtering changed");
  const auto missing = beatglass::library::browse(dir.path / "missing");
  require(missing.entries.empty() && !missing.diagnostic.empty(),
          "missing directory was not reported atomically");
  const auto file_as_directory = beatglass::library::browse(dir.path / "notes.txt");
  require(file_as_directory.entries.empty() && !file_as_directory.diagnostic.empty(),
          "file path was not rejected as a catalog directory");
  require(beatglass::library::browse(dir.path, "ambient").entries.size() == 1,
          "FLAC extension was not discovered case-insensitively");
  require(beatglass::library::browse(dir.path, "live").entries.size() == 1,
          "AIFF extension was not discovered case-insensitively");
  require(beatglass::library::browse(dir.path, "take").entries.size() == 1,
          "AIF extension was not discovered case-insensitively");
  require(beatglass::library::browse(dir.path, "podcast").entries.empty(),
          "M4A extension was incorrectly advertised");
  require(beatglass::library::browse(dir.path, "voice").entries.empty(),
          "AAC extension was incorrectly advertised");
  std::filesystem::rename(dir.path / "Alpha.mp3", dir.path / "Alpha.txt");
  require(beatglass::library::browse(dir.path, "alpha").entries.empty(),
          "unsupported extension mutation remained visible");
  std::filesystem::rename(dir.path / "Alpha.txt", dir.path / "Alpha.mp3");
  require(beatglass::library::browse(dir.path, "alpha").entries.size() == 1,
          "supported extension restoration was not visible");
  std::ofstream(dir.path / "Alpha-copy.mp3") << "audio";
  require(beatglass::library::browse(dir.path, "alpha").entries.size() == 2,
          "renamed duplicate source was not represented");
  std::ofstream(dir.path / "ALPHA.mp3") << "audio";
  const auto tied = beatglass::library::browse(dir.path, "alpha");
  require(tied.entries.size() >= 2,
          "duplicate sources were not represented");
  for (const auto& entry : tied.entries) {
    require(!entry.name.empty() && !entry.path.empty() && entry.format == "MP3",
            "duplicate catalog entries lost identity metadata");
  }
  const auto empty = beatglass::library::browse(dir.path, "empty");
  require(empty.entries.size() == 1 && empty.entries[0].bytes == 0 && empty.entries[0].format == "WAV",
          "zero-byte supported source metadata was not retained");
  std::filesystem::remove(dir.path / "Alpha.mp3");
  std::filesystem::remove(dir.path / "Alpha-copy.mp3");
  std::filesystem::remove(dir.path / "ALPHA.mp3");
  const auto after_remove = beatglass::library::browse(dir.path, "alpha");
  require(after_remove.entries.empty() && after_remove.diagnostic.empty(),
          "removed catalog entry remained visible");
  std::ofstream(dir.path / "Beta.mp3") << "audio";
  const auto renamed_replacement = beatglass::library::browse(dir.path, "beta");
  require(renamed_replacement.entries.size() == 1 && renamed_replacement.entries[0].name == "Beta.mp3",
          "replacement source was not visible after catalog refresh");
  std::filesystem::remove(dir.path / "Beta.mp3");
  std::filesystem::create_directory(dir.path / "Beta.mp3");
  const auto directory_replacement = beatglass::library::browse(dir.path, "beta");
  require(directory_replacement.entries.size() == 1 && directory_replacement.entries[0].directory,
          "directory replacement was not visible after catalog refresh");
  std::filesystem::remove_all(dir.path / "Beta.mp3");
  std::filesystem::create_directory(dir.path / "Gamma.mp3");
  require(beatglass::library::browse(dir.path, "gamma").entries.size() == 1,
          "new directory replacement was not visible");
  std::filesystem::remove_all(dir.path / "Gamma.mp3");
  std::ofstream(dir.path / "Gamma.mp3") << "audio";
  const auto file_replacement = beatglass::library::browse(dir.path, "gamma");
  require(file_replacement.entries.size() == 1 && !file_replacement.entries[0].directory && file_replacement.entries[0].format == "MP3",
          "directory-to-file replacement was not classified as audio");
  std::filesystem::remove(dir.path / "Gamma.mp3");
  std::filesystem::remove_all(dir.path / "Mixes");
  std::filesystem::remove_all(dir.path / "Folder.WAV");
  const auto after_directory_remove = beatglass::library::browse(dir.path, "mix");
  require(after_directory_remove.entries.empty() && after_directory_remove.diagnostic.empty(),
          "removed navigation directory remained visible");
  return EXIT_SUCCESS;
}
