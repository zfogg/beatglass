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
    const CacheKey next_engine{key.content, "engine-v2", key.config, key.mode};
    cache.begin("next-engine", next_engine);
    assert(cache.publish("next-engine", next_engine, "engine-v2-data", {}, [] { return true; }) == Publish::committed);
    assert(cache.find(next_engine, live_result) && live_result == "engine-v2-data");
    assert(cache.find(key, live_result) == false);
    const CacheKey live_engine{key.content, "engine-v2", key.config, "live"};
    cache.begin("live-engine", live_engine);
    assert(cache.publish("live-engine", live_engine, "live-engine-data", {}, [] { return true; }) == Publish::committed);
    assert(cache.find(live_engine, live_result) && live_result == "live-engine-data");
    assert(cache.invalidate(live_engine));
    assert(!cache.find(live_engine, live_result));
    assert(cache.find(next_engine, live_result) && live_result == "engine-v2-data");
    const CacheKey next_config{key.content, key.engine, "config-b", key.mode};
    cache.begin("next-config", next_config);
    assert(cache.publish("next-config", next_config, "config-b-data", {}, [] { return true; }) == Publish::committed);
    assert(cache.find(next_config, live_result) && live_result == "config-b-data");
    const CacheKey live_config{key.content, key.engine, "config-b", "live"};
    cache.begin("live-config", live_config);
    assert(cache.publish("live-config", live_config, "live-config-data", {}, [] { return true; }) == Publish::committed);
    assert(cache.find(live_config, live_result) && live_result == "live-config-data");
    assert(cache.invalidate(live_config));
    assert(!cache.find(live_config, live_result));
    assert(cache.find(next_config, live_result) && live_result == "config-b-data");
    assert(cache.invalidate(next_config));
    assert(!cache.find(next_config, live_result));
    cache.begin("config-republish", next_config);
    assert(cache.publish("config-republish", next_config, "config-b-republished", {}, [] { return true; }) == Publish::committed);
    assert(cache.find(next_config, live_result) && live_result == "config-b-republished");
    cache.begin("stale-active", next_config);
    assert(cache.publish("stale-active", next_config, "discarded-stale", {}, [] { return false; }) == Publish::stale_input);
    assert(cache.find(next_config, live_result) && live_result == "config-b-republished");
    assert(cache.discard_retryable("stale-active"));
    assert(cache.find(next_config, live_result) && live_result == "config-b-republished");
    cache.begin("stale-engine", next_engine);
    assert(cache.publish("stale-engine", next_engine, "discarded-engine", {}, [] { return false; }) == Publish::stale_input);
    assert(cache.find(next_engine, live_result) && live_result == "engine-v2-data");
    assert(cache.discard_retryable("stale-engine"));
    assert(!cache.discard_retryable("stale-active"));
    assert(cache.find(next_config, live_result) && live_result == "config-b-republished");
    assert(cache.find(next_engine, live_result) && live_result == "engine-v2-data");
    const CacheKey next_content{"sha256:two", key.engine, key.config, key.mode};
    cache.begin("next-content", next_content);
    assert(cache.publish("next-content", next_content, "content-two-data", {}, [] { return true; }) == Publish::committed);
    assert(cache.find(next_content, live_result) && live_result == "content-two-data");
    const CacheKey live_content{"sha256:two", key.engine, key.config, "live"};
    cache.begin("live-content", live_content);
    assert(cache.publish("live-content", live_content, "live-content-data", {}, [] { return true; }) == Publish::committed);
    assert(cache.find(live_content, live_result) && live_result == "live-content-data");
    assert(cache.invalidate(live_content));
    assert(!cache.find(live_content, live_result));
    assert(cache.find(next_content, live_result) && live_result == "content-two-data");
    const CacheKey reciprocal_engine{"sha256:reciprocal-engine", "engine-v3", key.config, key.mode};
    const CacheKey reciprocal_engine_live{"sha256:reciprocal-engine", "engine-v3", key.config, "live"};
    cache.begin("reciprocal-engine", reciprocal_engine);
    cache.begin("reciprocal-engine-live", reciprocal_engine_live);
    assert(cache.publish("reciprocal-engine", reciprocal_engine, "offline-engine", {}, [] { return true; }) == Publish::committed);
    assert(cache.publish("reciprocal-engine-live", reciprocal_engine_live, "live-engine", {}, [] { return true; }) == Publish::committed);
    assert(cache.invalidate(reciprocal_engine));
    assert(!cache.find(reciprocal_engine, live_result));
    assert(cache.find(reciprocal_engine_live, live_result) && live_result == "live-engine");
    cache.begin("reciprocal-engine-republish", reciprocal_engine);
    assert(cache.publish("reciprocal-engine-republish", reciprocal_engine, "offline-engine-republished", {}, [] { return true; }) == Publish::committed);
    assert(cache.invalidate(reciprocal_engine_live));
    assert(!cache.find(reciprocal_engine_live, live_result));
    assert(!cache.invalidate(reciprocal_engine_live));
    assert(cache.find(reciprocal_engine, live_result) && live_result == "offline-engine-republished");
    const CacheKey reciprocal_config{"sha256:reciprocal-config", key.engine, "config-c", key.mode};
    const CacheKey reciprocal_config_live{"sha256:reciprocal-config", key.engine, "config-c", "live"};
    cache.begin("reciprocal-config", reciprocal_config);
    cache.begin("reciprocal-config-live", reciprocal_config_live);
    assert(cache.publish("reciprocal-config", reciprocal_config, "offline-config", {}, [] { return true; }) == Publish::committed);
    assert(cache.publish("reciprocal-config-live", reciprocal_config_live, "live-config", {}, [] { return true; }) == Publish::committed);
    assert(cache.invalidate(reciprocal_config));
    assert(!cache.find(reciprocal_config, live_result));
    assert(cache.find(reciprocal_config_live, live_result) && live_result == "live-config");
    cache.begin("reciprocal-config-republish", reciprocal_config);
    assert(cache.publish("reciprocal-config-republish", reciprocal_config, "offline-config-republished", {}, [] { return true; }) == Publish::committed);
    assert(cache.invalidate(reciprocal_config_live));
    assert(!cache.find(reciprocal_config_live, live_result));
    assert(!cache.invalidate(reciprocal_config_live));
    assert(cache.find(reciprocal_config, live_result) && live_result == "offline-config-republished");
    const CacheKey reciprocal_content{"sha256:reciprocal-content", key.engine, key.config, key.mode};
    const CacheKey reciprocal_content_live{"sha256:reciprocal-content", key.engine, key.config, "live"};
    cache.begin("reciprocal-content", reciprocal_content);
    cache.begin("reciprocal-content-live", reciprocal_content_live);
    assert(cache.publish("reciprocal-content", reciprocal_content, "offline-content", {}, [] { return true; }) == Publish::committed);
    assert(cache.publish("reciprocal-content-live", reciprocal_content_live, "live-content", {}, [] { return true; }) == Publish::committed);
    assert(cache.invalidate(reciprocal_content));
    assert(!cache.find(reciprocal_content, live_result));
    assert(cache.find(reciprocal_content_live, live_result) && live_result == "live-content");
    cache.begin("reciprocal-content-republish", reciprocal_content);
    assert(cache.publish("reciprocal-content-republish", reciprocal_content, "offline-content-republished", {}, [] { return true; }) == Publish::committed);
    assert(cache.invalidate(reciprocal_content_live));
    assert(!cache.find(reciprocal_content_live, live_result));
    assert(!cache.invalidate(reciprocal_content_live));
    assert(cache.find(reciprocal_content, live_result) && live_result == "offline-content-republished");
    cache.begin("complete", key);
    assert(cache.publish("complete", key, "result-a", {}, [] { return true; }) == Publish::committed);
    std::string result;
    assert(cache.find(key, result) && result == "result-a");
    assert(cache.invalidate(key));
    assert(!cache.find(key, result));
    assert(cache.find(live_key, result) && result == "live-data");
    assert(cache.invalidate(live_key));
    assert(!cache.find(live_key, result));
    assert(!cache.invalidate(live_key));
    assert(cache.find(next_config, result) && result == "config-b-data");
    assert(cache.invalidate(next_config));
    assert(!cache.find(next_config, result));
    assert(cache.find(next_engine, result) && result == "engine-v2-data");
    assert(cache.invalidate(next_content));
    assert(!cache.find(next_content, result));
    assert(!cache.invalidate(next_content));
    assert(cache.find(next_engine, result) && result == "engine-v2-data");
    cache.begin("content-republish", next_content);
    assert(cache.publish("content-republish", next_content, "content-two-republished", {}, [] { return true; }) == Publish::committed);
    assert(cache.find(next_content, result) && result == "content-two-republished");
    assert(cache.invalidate(next_content));
    assert(!cache.find(next_content, result));
    assert(cache.find(next_config, result) && result == "config-b-republished");
    assert(cache.invalidate(next_engine));
    assert(!cache.find(next_engine, result));
    cache.begin("engine-republish", next_engine);
    assert(cache.publish("engine-republish", next_engine, "engine-v2-republished", {}, [] { return true; }) == Publish::committed);
    assert(cache.find(next_engine, result) && result == "engine-v2-republished");
    assert(cache.invalidate(next_engine));
    assert(!cache.find(next_engine, result));
    assert(cache.find(next_config, result) && result == "config-b-republished");
    cache.begin("republish", key);
    assert(cache.publish("republish", key, "result-b", {}, [] { return true; }) == Publish::committed);
    assert(cache.find(key, result) && result == "result-b");
    assert(cache.invalidate(key));
    assert(!cache.find(key, result));
    assert(cache.find(next_config, result) && result == "config-b-republished");
    cache.begin("republish", key);
    assert(cache.publish("republish", key, "duplicate-result", {}, [] { return true; }) == Publish::error);
    assert(!cache.find(key, result));
    const CacheKey unknown{"sha256:unknown", key.engine, key.config, key.mode};
    assert(!cache.invalidate(unknown));
    cache.begin("stale", unknown);
    assert(cache.publish("stale", unknown, "stale-result", {}, [] { return false; }) == Publish::stale_input);
    const auto stale_runs = cache.retryable_runs();
    assert(stale_runs.size() == 1 && stale_runs.front().id == "stale");
    assert(cache.discard_retryable("stale"));
    const CacheKey rollback_key{"sha256:rollback", key.engine, key.config, key.mode};
    cache.begin("rollback", rollback_key);
    bool verifier_threw = false;
    try {
      (void)cache.publish("rollback", rollback_key, "discarded-result", {}, []() -> bool { throw std::runtime_error("verifier failed"); });
    } catch (...) { verifier_threw = true; }
    assert(verifier_threw);
    const auto rollback_retryable = cache.retryable_runs();
    assert(rollback_retryable.size() == 1 && rollback_retryable.front().id == "rollback");
    assert(cache.publish("rollback", rollback_key, "recovered-after-rollback", {}, [] { return true; }) == Publish::committed);
    assert(cache.find(rollback_key, result) && result == "recovered-after-rollback");
    assert(cache.invalidate(rollback_key));
    cache.begin("stale-recover", unknown);
    assert(cache.publish("stale-recover", unknown, "recovered-result", {}, [] { return false; }) == Publish::stale_input);
    assert(cache.publish("stale-recover", unknown, "recovered-result", {}, [] { return true; }) == Publish::committed);
    assert(cache.find(unknown, result) && result == "recovered-result");
    assert(cache.invalidate(unknown));
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
    assert(cache.publish("cancelled", cancelled_key, "recovered-cancelled", {}, [] { return true; }) == Publish::committed);
    assert(cache.find(cancelled_key, result) && result == "recovered-cancelled");
    assert(cache.invalidate(cancelled_key));
    assert(!cache.discard_retryable("cancelled"));
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
    const CacheKey interrupted_key{"sha256:interrupted", key.engine, key.config, key.mode};
    cache.begin("zz-crash-recovery", interrupted_key);
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
    assert(remaining.size() == 3 && remaining[0].id == "a-retry" && remaining[1].id == "z-retry" && remaining[2].id == "zz-crash-recovery");
    assert(reopened.discard_retryable("a-retry"));
    assert(reopened.discard_retryable("z-retry"));
    const CacheKey interrupted_key{"sha256:interrupted", key.engine, key.config, key.mode};
    reopened.begin("zz-crash-recovery", interrupted_key);
    assert(reopened.publish("zz-crash-recovery", interrupted_key, "recovered-after-reopen", {}, [] { return true; }) == Publish::committed);
    assert(reopened.find(interrupted_key, result) && result == "recovered-after-reopen");
    assert(reopened.invalidate(interrupted_key));
    assert(!reopened.discard_retryable("zz-crash-recovery"));
    assert(reopened.retryable_runs().empty());
  }
  {
    SqliteCache reopened_again(path.string());
    assert(reopened_again.retryable_runs().empty());
    std::string result;
    const CacheKey retained_config{key.content, key.engine, "config-b", key.mode};
    assert(reopened_again.find(retained_config, result) && result == "config-b-republished");
    const CacheKey durable_engine{"sha256:durable-engine", "engine-durable", key.config, key.mode};
    const CacheKey durable_content{"sha256:durable-content", key.engine, key.config, key.mode};
    reopened_again.begin("durable-engine", durable_engine);
    reopened_again.begin("durable-content", durable_content);
    assert(reopened_again.publish("durable-engine", durable_engine, "engine-result", {}, [] { return true; }) == Publish::committed);
    assert(reopened_again.publish("durable-content", durable_content, "content-result", {}, [] { return true; }) == Publish::committed);
    assert(reopened_again.invalidate(durable_engine));
    assert(reopened_again.invalidate(durable_content));
    assert(!reopened_again.find(durable_engine, result));
    assert(!reopened_again.find(durable_content, result));
    assert(reopened_again.invalidate(retained_config));
    assert(!reopened_again.invalidate(retained_config));
    assert(!reopened_again.find(retained_config, result));
  }
  {
    SqliteCache reopened_third(path.string());
    assert(reopened_third.retryable_runs().empty());
    std::string result;
    const CacheKey durable_engine{"sha256:durable-engine", "engine-durable", key.config, key.mode};
    const CacheKey durable_content{"sha256:durable-content", key.engine, key.config, key.mode};
    const CacheKey retained_config{key.content, key.engine, "config-b", key.mode};
    assert(!reopened_third.find(durable_engine, result));
    assert(!reopened_third.find(durable_content, result));
    assert(!reopened_third.find(retained_config, result));
  }
  std::filesystem::remove(path, ec);
}
