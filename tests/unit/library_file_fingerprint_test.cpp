#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/detail/file_fingerprint.hpp"
#include <chrono>
#include <fstream>
#include <stdexcept>
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
  REQUIRE(b.size == a.size);
  const bool modification_time_preserved = b.modified == a.modified;
  REQUIRE(modification_time_preserved);
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

TEST_CASE("file fingerprint handles empty and buffer boundary lengths") {
  TempFile f;
  for (const std::size_t size : {std::size_t{0}, std::size_t{65535}, std::size_t{65536}, std::size_t{65537}, std::size_t{131072}}) {
    f.write(std::string(size, 'x')); std::uintmax_t observed = 0;
    const auto result = fingerprint_file(f.path, {}, [&](auto count) { observed = count; });
    REQUIRE(result.status == FingerprintStatus::ok); REQUIRE(result.size == size); REQUIRE(observed == size);
  }
  f.write(""); REQUIRE(fingerprint_file(f.path).digest == "sha256:e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}
TEST_CASE("file fingerprint honors stop from final observer and detects truncation") {
  TempFile f; f.write(std::string(65537, 'x')); std::stop_source stop;
  auto cancelled = fingerprint_file(f.path, stop.get_token(), [&](auto n) { if (n == 65537) stop.request_stop(); });
  REQUIRE(cancelled.status == FingerprintStatus::cancelled);
  f.write(std::string(140000, 'x'));
  bool truncated = false;
  auto shortened = fingerprint_file(f.path, {}, [&](auto) { if (!truncated) { truncated = true; std::filesystem::resize_file(f.path, 65536); } });
  REQUIRE(shortened.status == FingerprintStatus::changed);
}
TEST_CASE("file fingerprint reports observer exceptions") {
  TempFile f; f.write("abc");
  auto failed = fingerprint_file(f.path, {}, [](auto) { throw std::runtime_error("observer failed"); });
  REQUIRE(failed.status == FingerprintStatus::error); REQUIRE(failed.diagnostic == "observer failed");
}
TEST_CASE("file fingerprint statuses have stable labels") {
 CHECK(std::string(fingerprint_status_label(FingerprintStatus::ok)) == "ok");
 CHECK(std::string(fingerprint_status_label(FingerprintStatus::cancelled)) == "cancelled");
 CHECK(std::string(fingerprint_status_label(FingerprintStatus::changed)) == "changed");
 CHECK(std::string(fingerprint_status_label(FingerprintStatus::error)) == "error");
}

TEST_CASE("file fingerprint retry policy distinguishes transient outcomes") {
 CHECK(fingerprint_status_is_retryable(FingerprintStatus::cancelled));
 CHECK(fingerprint_status_is_retryable(FingerprintStatus::error));
 CHECK_FALSE(fingerprint_status_is_retryable(FingerprintStatus::ok));
 CHECK_FALSE(fingerprint_status_is_retryable(FingerprintStatus::changed));
}

TEST_CASE("file fingerprint refresh policy isolates changed sources") {
 CHECK(fingerprint_status_requires_refresh(FingerprintStatus::changed));
 CHECK_FALSE(fingerprint_status_requires_refresh(FingerprintStatus::ok));
 CHECK_FALSE(fingerprint_status_requires_refresh(FingerprintStatus::cancelled));
 CHECK_FALSE(fingerprint_status_requires_refresh(FingerprintStatus::error));
}

TEST_CASE("file fingerprint diagnostics preserve stable status labels") {
 CHECK(fingerprint_status_diagnostic(FingerprintStatus::changed) == "changed");
 CHECK(fingerprint_status_diagnostic(FingerprintStatus::error, "cannot open") ==
       "error: cannot open");
}
