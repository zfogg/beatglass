#include "beatglass/library/storage_owner.hpp"
#include <cassert>
#include <filesystem>

int main() {
  using namespace beatglass::library;
  const auto path = std::filesystem::temp_directory_path() / "beatglass-storage-owner.sqlite";
  std::error_code ec;
  std::filesystem::remove(path, ec);
  std::string diagnostic;
  StorageOwner owner;
  assert(!owner.open(StorageConfig{}, &diagnostic));
  assert(!diagnostic.empty() && owner.cache() == nullptr);
  assert(owner.open(StorageConfig{path}, &diagnostic));
  const beatglass::library_detail::CacheKey key{"sha256:owner", "engine", "config", "offline"};
  owner.cache()->begin("recovery", key);
  owner.close();
  assert(owner.retryable_runs().empty());
  assert(owner.open(StorageConfig{path}));
  const auto retryable = owner.retryable_runs();
  assert(retryable.size() == 1 && retryable.front().id == "recovery");
  assert(owner.discard_retryable("recovery"));
  assert(owner.retryable_runs().empty());
  owner.close();
  std::filesystem::remove(path, ec);
}
