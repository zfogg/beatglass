#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/catalog.hpp"
#include <chrono>
#include <fstream>

namespace {
struct TempDir {
  std::filesystem::path path = std::filesystem::temp_directory_path() /
    ("beatglass-catalog-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  TempDir() { std::filesystem::create_directory(path); }
  ~TempDir() { std::error_code ec; std::filesystem::remove_all(path, ec); }
  void file(const char* name) { std::ofstream(path / name) << "audio"; }
};
}

TEST_CASE("catalog lists folders and supported audio in deterministic order") {
  TempDir dir; dir.file("zeta.WAV"); dir.file("Alpha.mp3"); dir.file("notes.txt");
  std::filesystem::create_directory(dir.path / "Mixes");
  const auto result = beatglass::library::browse(dir.path);
  REQUIRE(result.diagnostic.empty());
  REQUIRE(result.entries.size() == 3);
  CHECK(result.entries[0].name == "Alpha.mp3");
  CHECK(result.entries[1].name == "Mixes");
  CHECK(result.entries[2].name == "zeta.WAV");
  CHECK(result.entries[0].bytes == 5);
  CHECK(result.entries[0].format == "MP3");
  CHECK(result.entries[0].title == "Alpha");
  CHECK(result.entries[0].artist.empty());
  CHECK(result.entries[0].duration_seconds == -1.0);
  CHECK(result.entries[1].bytes == 0);
}

TEST_CASE("catalog applies case-insensitive name filtering and rejects missing folders") {
  TempDir dir; dir.file("Summer.flac"); dir.file("winter.ogg");
  const auto filtered = beatglass::library::browse(dir.path, "SUM");
  REQUIRE(filtered.entries.size() == 1);
  CHECK(filtered.entries[0].name == "Summer.flac");
  const auto missing = beatglass::library::browse(dir.path / "missing");
  CHECK(missing.entries.empty());
  CHECK_FALSE(missing.diagnostic.empty());
}
TEST_CASE("catalog does not advertise unqualified codecs") {
  TempDir dir; dir.file("stream.ogg"); dir.file("album.m4a"); dir.file("track.aiff");
  const auto result = beatglass::library::browse(dir.path);
  REQUIRE(result.entries.size() == 1);
  CHECK(result.entries[0].name == "track.aiff");
}

TEST_CASE("catalog normalizes the short AIFF extension") {
  TempDir dir; dir.file("voice.aif");
  const auto result = beatglass::library::browse(dir.path);
  REQUIRE(result.entries.size() == 1);
  CHECK(result.entries[0].format == "AIFF");
  CHECK(result.entries[0].title == "voice");
}
