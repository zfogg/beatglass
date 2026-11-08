#include "beatglass/capture/file_reader.hpp"
#include <cassert>
#include <filesystem>

int main() {
  using namespace beatglass;
  bool invalid_epoch = false;
  try { FileReader reader("missing.wav", 0); }
  catch (const std::invalid_argument&) { invalid_epoch = true; }
  assert(invalid_epoch);

  const auto unsupported = std::filesystem::temp_directory_path() / "beatglass-reader.m4a";
  bool classified = false;
  try { FileReader reader(unsupported, 1); }
  catch (const FileOpenError& error) {
    classified = error.kind() == FileOpenErrorKind::unsupported_format;
  }
  assert(classified);
}
