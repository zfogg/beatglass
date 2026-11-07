#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/detail/file_snapshot.hpp"
#include <chrono>
#include <cstdlib>
#include <array>
#include <filesystem>
#include <fstream>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
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
  TempDir temp; auto file = temp.path / "source"; write(file, "abc");
  auto opened = SnapshotOpen::create(file); INFO(opened.diagnostic); REQUIRE(opened.status == SnapshotStatus::ok); REQUIRE(opened.value);
  REQUIRE(opened.value->size() == 3);
  REQUIRE(opened.value->digest() == "sha256:ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
  std::array<std::byte, 3> data{}; auto read = opened.value->read_at(0, data);
  REQUIRE(read.status == SnapshotStatus::ok); REQUIRE(read.bytes == 3); REQUIRE(std::to_integer<char>(data[0]) == 'a');
#ifndef _WIN32
  REQUIRE(::pwrite(opened.value->borrowed_descriptor(), "x", 1, 0) == -1);
#endif
  REQUIRE(opened.value->verify() == SnapshotStatus::ok);
  write(file, "xyz"); REQUIRE(opened.value->verify() == SnapshotStatus::stale_input);
  read = opened.value->read_at(0, data); REQUIRE(read.status == SnapshotStatus::ok); REQUIRE(read.bytes == 3); REQUIRE(std::to_integer<char>(data[0]) == 'a');
}
TEST_CASE("private snapshot rejects path replacement and honors cancellation") {
  TempDir temp; auto file = temp.path / "source"; write(file, "abc");
  auto opened = SnapshotOpen::create(file); REQUIRE(opened.status == SnapshotStatus::ok);
  std::filesystem::rename(file, temp.path / "old"); write(file, "abc");
  REQUIRE(opened.value->verify() == SnapshotStatus::stale_input);
  std::stop_source stop; stop.request_stop();
  REQUIRE(SnapshotOpen::create(file, stop.get_token()).status == SnapshotStatus::cancelled);
  auto second = SnapshotOpen::create(file); REQUIRE(second.status == SnapshotStatus::ok);
  REQUIRE(second.value->verify(stop.get_token()) == SnapshotStatus::cancelled);
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
  int temporary_fd = -1;
#endif
  auto snapshot = SnapshotOpen::create(file, stop.get_token(), [&](auto) {
    ++notifications;
#ifdef __linux__
    for (const auto& entry : std::filesystem::directory_iterator("/proc/self/fd")) {
      std::error_code error; auto target = std::filesystem::read_symlink(entry.path(), error);
      if (!error && target.parent_path() == std::filesystem::temp_directory_path() && target.filename().string().starts_with("#") && target.filename().string().ends_with(" (deleted)")) temporary_fd = std::stoi(entry.path().filename().string());
    }
#endif
    stop.request_stop();
  });
  REQUIRE(notifications == 1); REQUIRE(snapshot.status == SnapshotStatus::cancelled); REQUIRE_FALSE(snapshot.value);
#ifdef __linux__
  REQUIRE(temporary_fd >= 0); errno = 0; REQUIRE(::fcntl(temporary_fd, F_GETFD) == -1); REQUIRE(errno == EBADF);
#endif
  REQUIRE(std::filesystem::file_size(file) == 200000);
#endif
}

#ifdef _WIN32
TEST_CASE("Windows snapshot fails closed on non-files and cancels copying") {
  TempDir temp; auto file = temp.path / L"\u97f3\u697d-source"; write(file, std::string(200000, 'x'));
  std::stop_source stop; int calls = 0;
  auto cancelled = SnapshotOpen::create(file, stop.get_token(), [&](auto) { ++calls; stop.request_stop(); });
  INFO(cancelled.diagnostic); REQUIRE(calls == 1); REQUIRE(cancelled.status == SnapshotStatus::cancelled); REQUIRE_FALSE(cancelled.value);
  auto opened = SnapshotOpen::create(file); INFO(opened.diagnostic); REQUIRE(opened.status == SnapshotStatus::ok);
  REQUIRE(opened.value->verify() == SnapshotStatus::ok);
  std::array<std::byte, 16> bytes{}; auto read = opened.value->read_at(0, bytes);
  REQUIRE(read.status == SnapshotStatus::ok); REQUIRE(read.bytes == bytes.size());
  REQUIRE(std::to_integer<char>(bytes[0]) == 'x');
  REQUIRE(opened.value->size() == 200000);
  auto eof = opened.value->read_at(opened.value->size(), bytes); REQUIRE(eof.status == SnapshotStatus::ok); REQUIRE(eof.bytes == 0);
  std::stop_source already; already.request_stop(); REQUIRE(opened.value->read_at(0, bytes, already.get_token()).status == SnapshotStatus::cancelled);
  REQUIRE(SnapshotOpen::create(temp.path).status != SnapshotStatus::ok);
}
#endif

#ifdef _WIN32
TEST_CASE("Windows snapshot denies write sharing and removes its temporary") {
  TempDir temp; auto file = temp.path / "source"; write(file, std::string(200000, 'x'));
  std::filesystem::path snapshot_path; bool denied = false;
  auto prefix = L"beatglass-snapshot-" + std::to_wstring(GetCurrentProcessId()) + L"-";
  {
    auto snapshot = SnapshotOpen::create(file, {}, [&](auto) {
      if (!snapshot_path.empty()) return;
      for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::temp_directory_path())) {
        if (entry.path().filename().wstring().starts_with(prefix)) {
          snapshot_path = entry.path();
          HANDLE h = CreateFileW(snapshot_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
          denied = h == INVALID_HANDLE_VALUE && GetLastError() == ERROR_SHARING_VIOLATION;
          if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
          break;
        }
      }
    });
    INFO(snapshot.diagnostic); REQUIRE(snapshot.status == SnapshotStatus::ok);
    REQUIRE_FALSE(snapshot_path.empty()); REQUIRE(denied); REQUIRE(std::filesystem::exists(snapshot_path));
  }
  REQUIRE_FALSE(std::filesystem::exists(snapshot_path));
}
#endif
#ifdef _WIN32
TEST_CASE("Windows snapshot retains bytes after original content restoration") {
  TempDir temp; auto file = temp.path / "source"; write(file, "abc");
  auto original_time = std::filesystem::last_write_time(file);
  auto snapshot = SnapshotOpen::create(file); INFO(snapshot.diagnostic); REQUIRE(snapshot.status == SnapshotStatus::ok);
  write(file, "xyz"); write(file, "abc"); std::filesystem::last_write_time(file, original_time);
  auto status = snapshot.value->verify(); REQUIRE((status == SnapshotStatus::ok || status == SnapshotStatus::stale_input));
  std::array<std::byte, 3> out{}; auto result = snapshot.value->read_at(0, out);
  REQUIRE(result.status == SnapshotStatus::ok); REQUIRE(result.bytes == 3); REQUIRE(std::to_integer<char>(out[0]) == 'a');
}
#endif

TEST_CASE("snapshot reports original modification time as Unix nanoseconds") {
  TempDir temp; auto file = temp.path / "source"; write(file, "abc");
  auto snapshot = SnapshotOpen::create(file); INFO(snapshot.diagnostic); REQUIRE(snapshot.status == SnapshotStatus::ok);
  auto actual = snapshot.value->modified_unix_ns(); REQUIRE(actual);
#ifdef _WIN32
  WIN32_FILE_ATTRIBUTE_DATA data{}; REQUIRE(GetFileAttributesExW(file.c_str(), GetFileExInfoStandard, &data));
  ULARGE_INTEGER ticks{}; ticks.LowPart = data.ftLastWriteTime.dwLowDateTime; ticks.HighPart = data.ftLastWriteTime.dwHighDateTime;
  REQUIRE(*actual == (static_cast<std::int64_t>(ticks.QuadPart) - 116444736000000000LL) * 100);
#else
  struct stat st{}; REQUIRE(::stat(file.c_str(), &st) == 0);
#ifdef __APPLE__
  REQUIRE(*actual == static_cast<std::int64_t>(st.st_mtimespec.tv_sec) * 1000000000LL + st.st_mtimespec.tv_nsec);
#else
  REQUIRE(*actual == static_cast<std::int64_t>(st.st_mtim.tv_sec) * 1000000000LL + st.st_mtim.tv_nsec);
#endif
#endif
}
#ifdef __linux__
TEST_CASE("Linux snapshot fails closed when anonymous temporary creation is unavailable") {
  TempDir temp; auto file = temp.path / "source"; write(file, "abc");
  const char* current = std::getenv("TMPDIR"); const bool had = current != nullptr; std::string old = current ? current : "";
  struct Restore { bool had; std::string value; ~Restore() { if (had) ::setenv("TMPDIR", value.c_str(), 1); else ::unsetenv("TMPDIR"); } } restore{had, old};
  REQUIRE(::setenv("TMPDIR", "/proc", 1) == 0);
  auto result = SnapshotOpen::create(file);
  REQUIRE(result.status == SnapshotStatus::unverifiable); REQUIRE_FALSE(result.value);
}
#endif

TEST_CASE("snapshot verification observes cancellation between hash chunks") {
  TempDir temp; auto file = temp.path / "source"; write(file, std::string(200000, 'x'));
  auto opened = SnapshotOpen::create(file); INFO(opened.diagnostic); REQUIRE(opened.status == SnapshotStatus::ok);
  std::stop_source stop; int callbacks = 0;
  auto result = opened.value->verify(stop.get_token(), [&](auto) { ++callbacks; stop.request_stop(); });
  REQUIRE(callbacks == 1); REQUIRE(result == SnapshotStatus::cancelled);
  REQUIRE(opened.value->verify() == SnapshotStatus::ok);
}

#ifdef __linux__
TEST_CASE("snapshot verification rejects altered snapshot bytes") {
  TempDir temp; auto file = temp.path / "source"; write(file, "abc");
  auto snapshot = SnapshotOpen::create(file); INFO(snapshot.diagnostic); REQUIRE(snapshot.status == SnapshotStatus::ok);
  const int descriptor = snapshot.value->borrowed_descriptor();
  REQUIRE(::fchmod(descriptor, S_IRUSR | S_IWUSR) == 0);
  auto proc = std::string("/proc/self/fd/") + std::to_string(descriptor);
  const int writer = ::open(proc.c_str(), O_WRONLY | O_CLOEXEC); REQUIRE(writer >= 0);
  REQUIRE(::pwrite(writer, "z", 1, 0) == 1); REQUIRE(::close(writer) == 0);
  REQUIRE(snapshot.value->verify() == SnapshotStatus::unverifiable);
}
#endif

#ifdef _WIN32
TEST_CASE("Windows restricted source sharing rejects writers and existing write handles") {
  TempDir temp; auto file = temp.path / L"restricted-source"; write(file, "source bytes");
  HANDLE existing = CreateFileW(file.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  REQUIRE(existing != INVALID_HANDLE_VALUE);
  auto blocked = SnapshotOpen::create(file, {}, {}, SnapshotSourcePolicy::deny_writes);
  REQUIRE(blocked.status == SnapshotStatus::unverifiable); REQUIRE(CloseHandle(existing));
  {
    auto opened = SnapshotOpen::create(file, {}, {}, SnapshotSourcePolicy::deny_writes);
    INFO(opened.diagnostic); REQUIRE(opened.status == SnapshotStatus::ok);
    HANDLE writer = CreateFileW(file.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE(writer == INVALID_HANDLE_VALUE); REQUIRE(GetLastError() == ERROR_SHARING_VIOLATION);
    HANDLE deleter = CreateFileW(file.c_str(), DELETE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE(deleter == INVALID_HANDLE_VALUE); REQUIRE(GetLastError() == ERROR_SHARING_VIOLATION);
    REQUIRE(opened.value->verify() == SnapshotStatus::ok);
  }
  HANDLE writer = CreateFileW(file.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  REQUIRE(writer != INVALID_HANDLE_VALUE); REQUIRE(CloseHandle(writer));
}
#else
TEST_CASE("unsupported source sharing restriction fails closed") {
  TempDir temp; auto file = temp.path / "source"; write(file, "abc");
  REQUIRE(SnapshotOpen::create(file, {}, {}, SnapshotSourcePolicy::deny_writes).status == SnapshotStatus::unverifiable);
}
#endif
