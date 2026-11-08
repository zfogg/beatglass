#include "beatglass/library/storage_config.hpp"

#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "storage config smoke failure: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}
}

int main() {
  using beatglass::library::StorageConfig;
  using beatglass::library::valid_storage_config;
  std::string diagnostic = "stale";
  require(!valid_storage_config({}, &diagnostic), "empty config was accepted");
  require(!diagnostic.empty(), "invalid config omitted its diagnostic");
  require(!valid_storage_config({std::filesystem::path("folder/")}, &diagnostic),
          "path without a filename was accepted");
  require(valid_storage_config(StorageConfig{"cache.sqlite"}),
          "file-named config was rejected");
  return EXIT_SUCCESS;
}
