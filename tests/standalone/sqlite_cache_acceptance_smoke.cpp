#include "beatglass/library/detail/sqlite_cache.hpp"
#include <cassert>
#include <filesystem>
#include <string>
#include <stop_token>

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
    assert(cache.invalidate(key));
    assert(!cache.find(key, result));
    cache.begin("republish", key);
    assert(cache.publish("republish", key, "result-b", {}, [] { return true; }) == Publish::committed);
    assert(cache.find(key, result) && result == "result-b");
    const CacheKey unknown{"sha256:unknown", key.engine, key.config, key.mode};
    assert(!cache.invalidate(unknown));
    cache.begin("stale", unknown);
    assert(cache.publish("stale", unknown, "stale-result", {}, [] { return false; }) == Publish::stale_input);
    const auto stale_runs = cache.retryable_runs();
    assert(stale_runs.size() == 1 && stale_runs.front().id == "stale");
    assert(cache.discard_retryable("stale"));
    const CacheKey cancelled_key{"sha256:cancelled", key.engine, key.config, key.mode};
    cache.begin("cancelled", cancelled_key);
    std::stop_source stop;
    stop.request_stop();
    assert(cache.publish("cancelled", cancelled_key, "cancelled-result", stop.get_token(), [] { return true; }) == Publish::cancelled);
    assert(!cache.find(cancelled_key, result));
    assert(!cache.invalidate(cancelled_key));
    const auto cancelled_runs = cache.retryable_runs();
    assert(cancelled_runs.size() == 1 && cancelled_runs.front().id == "cancelled");
    assert(cache.discard_retryable("cancelled"));
    assert(!cache.find(CacheKey{key.content, key.engine, "config-b", key.mode}, result));
    assert(!cache.find(CacheKey{"sha256:two", key.engine, key.config, key.mode}, result));
    cache.begin("z-retry", key);
    cache.begin("a-retry", key);
    cache.begin("m-retry", key);
    cache.begin("marked", key);
    assert(!cache.mark_retryable("marked", CacheKey{"sha256:wrong", key.engine, key.config, key.mode}));
    assert(cache.mark_retryable("marked", key));
    assert(cache.discard_retryable("marked"));
    assert(!cache.mark_retryable("marked", key));
  }
  {
    SqliteCache reopened(path.string());
    const auto retryable = reopened.retryable_runs(2);
    assert(retryable.size() == 2 && retryable[0].id == "a-retry" && retryable[1].id == "m-retry");
    bool invalid_limit = false;
    try { (void)reopened.retryable_runs(0); } catch (const std::invalid_argument&) { invalid_limit = true; }
    bool oversized_limit = false;
    try { (void)reopened.retryable_runs(10001); } catch (const std::invalid_argument&) { oversized_limit = true; }
    assert(invalid_limit && oversized_limit && !reopened.discard_retryable("missing-retry"));
    std::string result;
    assert(reopened.find(key, result) && result == "result-a");
    assert(reopened.discard_retryable("m-retry"));
    const auto remaining = reopened.retryable_runs();
    assert(remaining.size() == 2 && remaining[0].id == "a-retry" && remaining[1].id == "z-retry");
    assert(reopened.discard_retryable("a-retry"));
    assert(reopened.discard_retryable("z-retry"));
    assert(reopened.retryable_runs().empty());
  }
  std::filesystem::remove(path, ec);
}
