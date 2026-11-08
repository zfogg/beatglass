#include "beatglass/library/detail/sqlite_cache.hpp"
#include <cassert>
#include <filesystem>
#include <string>

using beatglass::library_detail::CacheKey;
using beatglass::library_detail::Publish;
using beatglass::library_detail::SqliteCache;

int main() {
  const auto path = std::filesystem::temp_directory_path() / "beatglass-cache-acceptance.sqlite";
  std::error_code ec;
  std::filesystem::remove(path, ec);
  const CacheKey key{"sha256:one", "engine-v1", "config-a", "offline"};
  {
    SqliteCache cache(path.string());
    cache.begin("complete", key);
    assert(cache.publish("complete", key, "result-a", {}, [] { return true; }) == Publish::committed);
    std::string result;
    assert(cache.find(key, result) && result == "result-a");
    assert(!cache.find(CacheKey{key.content, key.engine, "config-b", key.mode}, result));
    assert(!cache.find(CacheKey{"sha256:two", key.engine, key.config, key.mode}, result));
    cache.begin("interrupted", key);
  }
  {
    SqliteCache reopened(path.string());
    const auto retryable = reopened.retryable_runs();
    assert(retryable.size() == 1 && retryable.front().id == "interrupted");
    std::string result;
    assert(reopened.find(key, result) && result == "result-a");
    assert(reopened.discard_retryable("interrupted"));
    assert(reopened.retryable_runs().empty());
  }
  std::filesystem::remove(path, ec);
}
