#include "beatglass/library/detail/file_fingerprint.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "fingerprint smoke failure: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}
}

int main() {
  using namespace beatglass::library_detail;
  const auto path = std::filesystem::temp_directory_path() /
      ("beatglass-fingerprint-smoke-" + std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()));
  std::ofstream(path) << "abc";

  const auto result = fingerprint_file(path);
  require(result.status == FingerprintStatus::ok, "valid file was not hashed");
  require(result.size == 3, "hashed size was incorrect");
  require(result.digest == "sha256:ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
          "SHA-256 digest was incorrect");

  std::stop_source stop;
  stop.request_stop();
  const auto cancelled = fingerprint_file(path, stop.get_token());
  require(cancelled.status == FingerprintStatus::cancelled,
          "pre-cancelled fingerprint was not cancelled");
  const auto missing = fingerprint_file(path.string() + ".missing");
  require(missing.status == FingerprintStatus::error,
          "missing fingerprint did not report error");
  std::error_code ec;
  std::filesystem::remove(path, ec);
  return EXIT_SUCCESS;
}
