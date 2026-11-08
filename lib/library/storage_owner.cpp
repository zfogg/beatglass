#include "beatglass/library/storage_owner.hpp"
namespace beatglass::library {
bool StorageOwner::open(const StorageConfig& config, std::string* diagnostic) {
  close();
  if (!valid_storage_config(config, diagnostic)) return false;
  try { cache_ = std::make_unique<library_detail::SqliteCache>(config.database_path.string()); return true; }
  catch (const std::exception& e) { if (diagnostic) *diagnostic = e.what(); return false; }
}
void StorageOwner::close() noexcept { cache_.reset(); }
std::vector<library_detail::RetryableRun> StorageOwner::retryable_runs(std::size_t limit) const {
  return cache_ ? cache_->retryable_runs(limit) : std::vector<library_detail::RetryableRun>{};
}
bool StorageOwner::discard_retryable(const std::string& id) { return cache_ ? cache_->discard_retryable(id) : false; }
bool StorageOwner::find_cached(const AnalysisIdentity& identity, std::string& result) const {
  return cache_ && cache_->find({identity.content_hash, identity.engine_version, identity.configuration_id, identity.mode}, result);
}
bool StorageOwner::invalidate_cached(const AnalysisIdentity& identity) {
  return cache_ && cache_->invalidate({identity.content_hash, identity.engine_version, identity.configuration_id, identity.mode});
}
library_detail::Publish StorageOwner::publish_analysis(const std::string& run_id, const AnalysisOperation& operation,
                                                       std::stop_token stop) {
  if (!cache_) return library_detail::Publish::error;
  return publish_operation(*cache_, run_id, operation, stop);
}
CacheAdmission StorageOwner::admit_analysis(const std::string& run_id, const AnalysisIdentity& identity, const std::string& source_path, const std::string& decoder_snapshot) {
  if (!cache_) return {};
  return ::beatglass::library::admit_analysis(*cache_, run_id, identity, source_path, decoder_snapshot);
}
}
