#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/detail/file_fingerprint.hpp"
#include <chrono>
#include <fstream>
using namespace beatglass::library_detail;
namespace {
struct TempFile {
  std::filesystem::path path = std::filesystem::temp_directory_path() /
    ("beatglass-hash-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  ~TempFile() { std::error_code ignored; std::filesystem::remove(path, ignored); }
  void write(const std::string& data) { std::ofstream file(path, std::ios::binary | std::ios::trunc); file << data; }
};
}
TEST_CASE("file fingerprint detects content changes even with restored attributes") {
  TempFile f; f.write("abc");
  auto a = fingerprint_file(f.path); REQUIRE(a.status == FingerprintStatus::ok);
  REQUIRE(a.digest == "sha256:ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
  auto time = std::filesystem::last_write_time(f.path); f.write("xyz"); std::filesystem::last_write_time(f.path, time);
  auto b = fingerprint_file(f.path); REQUIRE(b.status == FingerprintStatus::ok); REQUIRE(b.digest != a.digest);
  REQUIRE(b.size == a.size); REQUIRE(b.modified == a.modified);
}
TEST_CASE("file fingerprint observes cancellation and changes during read") {
  TempFile f; f.write(std::string(140000, 'x'));
  std::stop_source stop;
  auto cancelled = fingerprint_file(f.path, stop.get_token(), [&](auto) { stop.request_stop(); });
  REQUIRE(cancelled.status == FingerprintStatus::cancelled);
  bool changed = false;
  auto result = fingerprint_file(f.path, {}, [&](auto) { if (!changed) { changed = true; std::ofstream out(f.path, std::ios::binary | std::ios::app); out << 'a'; } });
  REQUIRE(result.status == FingerprintStatus::changed);
}
