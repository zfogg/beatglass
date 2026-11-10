#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/catalog.hpp"
#include <chrono>
#include <fstream>
#include <algorithm>

namespace {
struct TempDir {
  std::filesystem::path path = std::filesystem::temp_directory_path() /
    ("beatglass-catalog-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  TempDir() { std::filesystem::create_directory(path); }
  ~TempDir() { std::error_code ec; std::filesystem::remove_all(path, ec); }
  void file(const char* name) { std::ofstream(path / name) << "audio"; }
};
}

TEST_CASE("catalog exposes canonical audio format policy") {
  CHECK(beatglass::library::audio_format_for_path("track.WAV") == "WAV");
  CHECK(beatglass::library::audio_format_for_path("track.aif") == "AIFF");
  CHECK(beatglass::library::audio_format_for_path("notes.txt").empty());
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

TEST_CASE("catalog normalizes uppercase short AIFF extension") {
  TempDir dir; dir.file("VOICE.AIF");
  const auto result = beatglass::library::browse(dir.path);
  REQUIRE(result.entries.size() == 1);
  CHECK(result.entries[0].format == "AIFF");
}

TEST_CASE("catalog filters directories and files case-insensitively") {
  TempDir dir; dir.file("Track.WAV"); std::filesystem::create_directory(dir.path / "Samples");
  const auto result = beatglass::library::browse(dir.path, "SAMP");
  REQUIRE(result.entries.size() == 1);
  CHECK(result.entries[0].directory);
  CHECK(result.entries[0].name == "Samples");
  CHECK(result.entries[0].format.empty());
}

TEST_CASE("catalog preserves multi-dot and spaced source identity") {
 TempDir dir; dir.file("Live.Set.v2.WAV"); dir.file("Morning Take.mp3");
 const auto result = beatglass::library::browse(dir.path, "TAKE");
 REQUIRE(result.entries.size() == 1);
 CHECK(result.entries[0].name == "Morning Take.mp3");
 CHECK(result.entries[0].title == "Morning Take");
 CHECK(result.entries[0].format == "MP3");
}

TEST_CASE("catalog keeps hidden supported sources discoverable") {
 TempDir dir; dir.file(".hidden.FLAC");
 const auto result = beatglass::library::browse(dir.path, "HIDDEN");
 REQUIRE(result.entries.size() == 1);
 CHECK(result.entries[0].name == ".hidden.FLAC");
 CHECK(result.entries[0].title == ".hidden");
 CHECK(result.entries[0].format == "FLAC");
}

TEST_CASE("catalog reports a replaced folder path without stale entries") {
 TempDir dir; std::filesystem::create_directory(dir.path / "Library.wav");
 const auto first = beatglass::library::browse(dir.path);
 REQUIRE(first.entries.size() == 1);
 CHECK(first.entries[0].directory);
 std::filesystem::remove_all(dir.path / "Library.wav");
 dir.file("Library.wav");
 const auto replaced = beatglass::library::browse(dir.path);
 REQUIRE(replaced.entries.size() == 1);
 CHECK_FALSE(replaced.entries[0].directory);
 CHECK(replaced.entries[0].name == "Library.wav");
 CHECK(replaced.entries[0].format == "WAV");
}

TEST_CASE("catalog reports a replaced audio file as a directory") {
 TempDir dir; dir.file("Session.wav");
 const auto first = beatglass::library::browse(dir.path, "SESSION");
 REQUIRE(first.entries.size() == 1);
 CHECK_FALSE(first.entries[0].directory);
 std::filesystem::remove(dir.path / "Session.wav");
 std::filesystem::create_directory(dir.path / "Session.wav");
 const auto replaced = beatglass::library::browse(dir.path, "session");
 REQUIRE(replaced.entries.size() == 1);
 CHECK(replaced.entries[0].directory);
 CHECK(replaced.entries[0].format.empty());
}

TEST_CASE("catalog orders case-fold collisions by original name and preserves paths") {
#if defined(_WIN32)
 // Windows filesystems commonly use case-insensitive identity, so these two
 // names cannot coexist as distinct directory entries on the local volume.
 return;
#else
 TempDir dir; dir.file("alpha.wav"); dir.file("Alpha.wav");
 const auto result = beatglass::library::browse(dir.path);
 REQUIRE(result.entries.size() == 2);
 CHECK(result.entries[0].name == "Alpha.wav");
 CHECK(result.entries[1].name == "alpha.wav");
 CHECK(result.entries[0].path == dir.path / "Alpha.wav");
 CHECK(result.entries[1].path == dir.path / "alpha.wav");
#endif
}

TEST_CASE("catalog excludes unsupported files from matching searches") {
 TempDir dir; dir.file("notes.txt"); dir.file("mix.wav");
 const auto result = beatglass::library::browse(dir.path, "notes");
 CHECK(result.entries.empty());
 CHECK(result.diagnostic.empty());
}

TEST_CASE("catalog search remains deterministic after replacing a filtered entry") {
 TempDir dir; dir.file("Take.wav"); dir.file("Take.txt");
 auto first = beatglass::library::browse(dir.path, "take");
 REQUIRE(first.entries.size() == 1);
 CHECK(first.entries[0].name == "Take.wav");
 std::filesystem::remove(dir.path / "Take.wav");
 dir.file("Take.mp3");
 const auto replaced = beatglass::library::browse(dir.path, "TAKE");
 REQUIRE(replaced.entries.size() == 1);
 CHECK(replaced.entries[0].name == "Take.mp3");
 CHECK(replaced.entries[0].format == "MP3");
}

TEST_CASE("catalog returns distinct case-sensitive paths for identical folded names") {
#if defined(_WIN32)
 return;
#else
 TempDir dir; dir.file("Song.WAV"); dir.file("song.WAV");
 const auto result = beatglass::library::browse(dir.path, "song");
 REQUIRE(result.entries.size() == 2);
 CHECK(result.entries[0].path != result.entries[1].path);
 CHECK(result.entries[0].name == "Song.WAV");
 CHECK(result.entries[1].name == "song.WAV");
#endif
}

TEST_CASE("catalog ignores unsupported files even with case-folded matches") {
 TempDir dir; dir.file("TAKE.TXT"); dir.file("Take.FLAC");
 const auto result = beatglass::library::browse(dir.path, "take");
 REQUIRE(result.entries.size() == 1);
 CHECK(result.entries[0].name == "Take.FLAC");
 CHECK(result.entries[0].format == "FLAC");
}

TEST_CASE("catalog preserves empty directories as navigable entries") {
 TempDir dir; std::filesystem::create_directory(dir.path / "Empty Mix");
 const auto result = beatglass::library::browse(dir.path, "empty");
 REQUIRE(result.entries.size() == 1);
 CHECK(result.entries[0].directory);
 CHECK(result.entries[0].name == "Empty Mix");
 CHECK(result.entries[0].bytes == 0);
 CHECK(result.entries[0].format.empty());
}

TEST_CASE("recursive catalog discovers supported descendants without following links") {
    TempDir temp;
    const auto root = temp.path / "recursive";
    std::filesystem::create_directory(root);
    std::filesystem::create_directories(root / "nested");
    std::ofstream(root / "nested" / "deep.WAV") << "x";
    std::ofstream(root / "nested" / "ignore.txt") << "x";
    const auto result = beatglass::library::browse_recursive(root);
    REQUIRE(result.diagnostic.empty());
    REQUIRE(result.entries.size() == 2);
    CHECK(result.entries[0].directory);
    CHECK(result.entries[1].format == "WAV");
}

TEST_CASE("catalog reports a file path as an invalid browse root") {
 TempDir dir; dir.file("track.wav");
 const auto result = beatglass::library::browse(dir.path / "track.wav");
 CHECK(result.entries.empty());
 CHECK_FALSE(result.diagnostic.empty());
 CHECK(result.status == beatglass::library::CatalogStatus::invalid_root);
}
TEST_CASE("catalog exposes root-relative identity paths") {
 TempDir dir; std::filesystem::create_directory(dir.path / "nested");
 dir.file("track.wav");
 const auto direct = beatglass::library::browse(dir.path);
 REQUIRE(direct.entries.size() == 2);
 CHECK(direct.entries[0].relative_path == direct.entries[0].path.filename());
 const auto recursive = beatglass::library::browse_recursive(dir.path);
 REQUIRE(recursive.entries.size() == 2);
 CHECK(recursive.entries.front().relative_path == std::filesystem::path("nested"));
}
TEST_CASE("catalog result identifies its normalized query root") {
 TempDir dir; const auto result = beatglass::library::browse(dir.path);
 REQUIRE(result.status == beatglass::library::CatalogStatus::ok);
 CHECK(result.query_root == std::filesystem::absolute(dir.path));
}
TEST_CASE("catalog relative identity is computed without throwing") {
 TempDir dir; dir.file("track.wav");
 const auto result = beatglass::library::browse(dir.path);
 REQUIRE(result.entries.size() == 1);
 CHECK(result.status == beatglass::library::CatalogStatus::ok);
 CHECK(result.entries.front().relative_path == std::filesystem::path("track.wav"));
}
TEST_CASE("recursive catalog ordering is rooted in relative identity") {
 TempDir dir; std::filesystem::create_directories(dir.path / "z");
 std::ofstream(dir.path / "z" / "track.wav") << "x";
 const auto result = beatglass::library::browse_recursive(dir.path);
 REQUIRE(result.entries.size() == 2);
 CHECK(result.entries[0].relative_path.generic_string() == "z");
 CHECK(result.entries[1].relative_path.generic_string() == "z/track.wav");
}
TEST_CASE("catalog cancellation returns no partial snapshot") {
 TempDir dir; dir.file("track.wav");
 std::stop_source source; source.request_stop();
 const auto result = beatglass::library::browse(dir.path, {}, 10000, source.get_token());
 CHECK(result.entries.empty());
 CHECK(result.cancelled);
 CHECK(result.diagnostic == "catalog query cancelled");
}
TEST_CASE("catalog exposes a stable status for entry limits") {
 TempDir dir; dir.file("one.wav"); dir.file("two.flac");
 const auto result = beatglass::library::browse(dir.path, {}, 1);
 CHECK(result.status == beatglass::library::CatalogStatus::entry_limit_exceeded);
}
TEST_CASE("catalog fails closed when the entry budget is exceeded") {
 TempDir dir; dir.file("one.wav"); dir.file("two.flac");
 const auto result = beatglass::library::browse(dir.path, {}, 1);
 CHECK(result.entries.empty());
 CHECK(result.diagnostic == "catalog entry limit exceeded");
}
TEST_CASE("recursive catalog applies the entry budget") {
 TempDir dir; const auto root = dir.path;
 std::filesystem::create_directories(root / "nested");
 std::ofstream(root / "nested" / "deep.wav") << "x";
 const auto result = beatglass::library::browse_recursive(root, {}, 1);
 CHECK(result.entries.empty());
 CHECK(result.diagnostic == "catalog entry limit exceeded");
}
TEST_CASE("catalog skips symlinked files and directories") {
  TempDir dir; dir.file("real.wav");
  std::error_code ec;
  std::filesystem::create_symlink(dir.path / "real.wav", dir.path / "linked.wav", ec);
  if (ec) return; // Platform qualification may disable symlink creation.
  std::filesystem::create_directory_symlink(dir.path.parent_path(), dir.path / "linked-nested", ec);
  if (ec) return; // Directory symlink creation requires additional Windows privileges.
  ec.clear();
  if (!std::filesystem::is_symlink(std::filesystem::symlink_status(dir.path / "linked-nested", ec)) || ec) return;
  const auto result = beatglass::library::browse(dir.path);
  REQUIRE(result.entries.size() == 2);
  CHECK(std::ranges::any_of(result.entries, [](const auto& entry) { return entry.name == "real.wav"; }));
  CHECK(std::ranges::any_of(result.entries, [](const auto& entry) { return entry.name == "nested"; }));
  CHECK_FALSE(std::ranges::any_of(result.entries, [](const auto& entry) { return entry.name == "linked.wav" || entry.name == "linked-nested"; }));
}
