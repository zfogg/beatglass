#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/detail/file_snapshot.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#endif
using namespace beatglass::library_detail;
namespace {
struct TempDir {
  std::filesystem::path path = std::filesystem::temp_directory_path() / ("beatglass-snapshot-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  TempDir() { std::filesystem::create_directory(path); }
  ~TempDir() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
};
void write(const std::filesystem::path& p, const std::string& value) { std::ofstream out(p, std::ios::binary | std::ios::trunc); out << value; }
}
TEST_CASE("private snapshot retains read-only bytes and detects source changes") {
#ifdef _WIN32
  auto unavailable = SnapshotOpen::create("unused"); REQUIRE(unavailable.status == SnapshotStatus::unverifiable);
#else
  TempDir temp; auto file = temp.path / "source"; write(file, "abc");
  auto opened = SnapshotOpen::create(file); INFO(opened.diagnostic); REQUIRE(opened.status == SnapshotStatus::ok); REQUIRE(opened.value);
  REQUIRE(opened.value->digest() == "sha256:ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
  char data[3]{}; REQUIRE(::pread(opened.value->borrowed_descriptor(), data, 3, 0) == 3); REQUIRE(std::string(data, 3) == "abc");
  REQUIRE(::pwrite(opened.value->borrowed_descriptor(), "x", 1, 0) == -1);
  REQUIRE(opened.value->verify() == SnapshotStatus::ok);
  write(file, "xyz"); REQUIRE(opened.value->verify() == SnapshotStatus::stale_input);
  REQUIRE(::pread(opened.value->borrowed_descriptor(), data, 3, 0) == 3); REQUIRE(std::string(data, 3) == "abc");
#endif
}
TEST_CASE("private snapshot rejects path replacement and honors cancellation") {
#ifndef _WIN32
  TempDir temp; auto file = temp.path / "source"; write(file, "abc");
  auto opened = SnapshotOpen::create(file); REQUIRE(opened.status == SnapshotStatus::ok);
  std::filesystem::rename(file, temp.path / "old"); write(file, "abc");
  REQUIRE(opened.value->verify() == SnapshotStatus::stale_input);
  std::stop_source stop; stop.request_stop();
  REQUIRE(SnapshotOpen::create(file, stop.get_token()).status == SnapshotStatus::cancelled);
  auto second = SnapshotOpen::create(file); REQUIRE(second.status == SnapshotStatus::ok);
  REQUIRE(second.value->verify(stop.get_token()) == SnapshotStatus::cancelled);
#endif
}

TEST_CASE("snapshot retains bytes through source restoration and closes its descriptor") {
#ifndef _WIN32
  TempDir temp; auto file = temp.path / "source"; write(file, "abc"); int borrowed = -1;
  {
    auto original_time = std::filesystem::last_write_time(file);
    auto snapshot = SnapshotOpen::create(file); REQUIRE(snapshot.status == SnapshotStatus::ok);
    borrowed = snapshot.value->borrowed_descriptor();
    write(file, "xyz"); write(file, "abc"); std::filesystem::last_write_time(file, original_time);
    const auto status = snapshot.value->verify();
    REQUIRE((status == SnapshotStatus::ok || status == SnapshotStatus::stale_input));
    char contents[3]{}; REQUIRE(::pread(borrowed, contents, 3, 0) == 3); REQUIRE(std::string(contents, 3) == "abc");
  }
  errno = 0; REQUIRE(::fcntl(borrowed, F_GETFD) == -1); REQUIRE(errno == EBADF);
#endif
}
TEST_CASE("snapshot copying observes cancellation between chunks") {
#ifndef _WIN32
  TempDir temp; auto file = temp.path / "source"; write(file, std::string(200000, 'x'));
  std::stop_source stop; int notifications = 0;
#ifdef __linux__
  std::filesystem::path temporary;
#endif
  auto snapshot = SnapshotOpen::create(file, stop.get_token(), [&](auto) {
    ++notifications;
#ifdef __linux__
    for (const auto& entry : std::filesystem::directory_iterator("/proc/self/fd")) {
      std::error_code error; auto target = std::filesystem::read_symlink(entry.path(), error);
      if (!error && target.filename().string().starts_with("beatglass-snapshot-")) temporary = target;
    }
#endif
    stop.request_stop();
  });
  REQUIRE(notifications == 1); REQUIRE(snapshot.status == SnapshotStatus::cancelled); REQUIRE_FALSE(snapshot.value);
#ifdef __linux__
  REQUIRE_FALSE(temporary.empty()); REQUIRE_FALSE(std::filesystem::exists(temporary));
#endif
  REQUIRE(std::filesystem::file_size(file) == 200000);
#endif
}
