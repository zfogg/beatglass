#include "beatglass/library/analysis_commit.hpp"
#include <exception>
namespace beatglass::library {
bool reuse_cached_operation(library_detail::SqliteCache& cache, const AnalysisIdentity& identity, std::string& result) {
    return cache.find({identity.content_hash, identity.engine_version, identity.configuration_id, identity.mode}, result);
}
library_detail::Publish publish_operation(library_detail::SqliteCache& cache,const std::string& id,const AnalysisOperation& op,std::stop_token stop){ if(id.empty()||!op.valid()) return library_detail::Publish::error; if(stop.stop_requested()) return library_detail::Publish::cancelled; auto verifier=[&]{return op.verify(stop).outcome==FileVerificationOutcome::match;}; library_detail::CacheKey key{op.identity.content_hash,op.identity.engine_version,op.identity.configuration_id,op.identity.mode}; try { auto result = op.serialize(); if(stop.stop_requested()) return library_detail::Publish::cancelled; return cache.publish(id,key,std::move(result),stop,verifier); } catch (...) { try { cache.mark_retryable(id, key); } catch (...) {} return library_detail::Publish::error; } }
}
