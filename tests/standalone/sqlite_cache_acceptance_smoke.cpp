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
    bool empty_run_rejected = false;
    try { cache.begin("", key); } catch (const std::invalid_argument&) { empty_run_rejected = true; }
    bool empty_content_rejected = false;
    try { cache.begin("invalid-key", CacheKey{"", key.engine, key.config, key.mode}); }
    catch (const std::invalid_argument&) { empty_content_rejected = true; }
    bool long_run_rejected = false;
    try { cache.begin(std::string(4097, 'r'), key); } catch (const std::invalid_argument&) { long_run_rejected = true; }
    bool long_config_rejected = false;
    try { cache.begin("long-config", CacheKey{key.content, key.engine, std::string(4097, 'c'), key.mode}); }
    catch (const std::invalid_argument&) { long_config_rejected = true; }
    bool long_content_rejected = false;
    try { cache.begin("long-content", CacheKey{std::string(4097, 'h'), key.engine, key.config, key.mode}); }
    catch (const std::invalid_argument&) { long_content_rejected = true; }
    bool long_engine_rejected = false;
    try { cache.begin("long-engine", CacheKey{key.content, std::string(4097, 'e'), key.config, key.mode}); }
    catch (const std::invalid_argument&) { long_engine_rejected = true; }
    bool long_mode_rejected = false;
    try { cache.begin("long-mode", CacheKey{key.content, key.engine, key.config, std::string(4097, 'm')}); }
    catch (const std::invalid_argument&) { long_mode_rejected = true; }
    bool invalid_mode_rejected = false;
    try { cache.begin("invalid-mode", CacheKey{key.content, key.engine, key.config, "unknown"}); }
    catch (const std::invalid_argument&) { invalid_mode_rejected = true; }
    bool empty_engine_rejected = false;
    try { cache.begin("empty-engine", CacheKey{key.content, "", key.config, key.mode}); }
    catch (const std::invalid_argument&) { empty_engine_rejected = true; }
    bool empty_config_rejected = false;
    try { cache.begin("empty-config", CacheKey{key.content, key.engine, "", key.mode}); }
    catch (const std::invalid_argument&) { empty_config_rejected = true; }
    bool empty_mode_rejected = false;
    try { cache.begin("empty-mode", CacheKey{key.content, key.engine, key.config, ""}); }
    catch (const std::invalid_argument&) { empty_mode_rejected = true; }
    assert(empty_run_rejected && empty_content_rejected && long_run_rejected && long_config_rejected && long_content_rejected && long_engine_rejected && long_mode_rejected && invalid_mode_rejected && empty_engine_rejected && empty_config_rejected && empty_mode_rejected);
    const CacheKey live_key{key.content, key.engine, key.config, "live"};
    cache.begin("live-result", live_key);
    assert(cache.publish("live-result", live_key, "live-data", {}, [] { return true; }) == Publish::committed);
    std::string live_result;
    assert(cache.find(live_key, live_result) && live_result == "live-data");
    assert(!cache.find(key, live_result));
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
    bool empty_id_rejected = false;
    try { (void)cache.discard_retryable(""); } catch (const std::invalid_argument&) { empty_id_rejected = true; }
    bool nul_id_rejected = false;
    try { (void)cache.discard_retryable(std::string("bad\0id", 6)); } catch (const std::invalid_argument&) { nul_id_rejected = true; }
    assert(empty_id_rejected && nul_id_rejected);
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
