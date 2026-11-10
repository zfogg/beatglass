#include "beatglass/library/storage_owner.hpp"
#if defined(BEATGLASS_LIBRARY_FINGERPRINT)
#include "beatglass/library/detail/cache_identity.hpp"
#endif
#include <stdexcept>
namespace beatglass::library {
bool StorageOwner::open(const StorageConfig& config, std::string* diagnostic) {
  close();
  if (diagnostic) diagnostic->clear();
  if (!valid_storage_config(config, diagnostic)) return false;
  try { cache_ = std::make_unique<library_detail::SqliteCache>(config.database_path.string()); return true; }
  catch (const std::exception& e) {
    // A failed reopen must never leave a partially constructed cache usable.
    cache_.reset();
    if (diagnostic) *diagnostic = e.what();
    return false;
  }
}
void StorageOwner::close() noexcept { cache_.reset(); }
bool StorageOwner::refresh_collection_file(const std::filesystem::path& path, std::string* diagnostic) {
#if !defined(BEATGLASS_LIBRARY_FINGERPRINT)
  (void)path;
  if (diagnostic) *diagnostic = "file fingerprint support is disabled";
  return false;
#else
  if (!cache_) {
    if (diagnostic) *diagnostic = "storage is closed";
    return false;
  }
  try {
    return library_detail::refresh_collection_file(*cache_, path, {}, diagnostic) == library_detail::FingerprintStatus::ok;
  } catch (const std::exception& e) {
    if (diagnostic) *diagnostic = e.what();
    return false;
  }
#endif
}
std::vector<library_detail::RetryableRun> StorageOwner::retryable_runs(std::size_t limit) const {
  return cache_ ? cache_->retryable_runs(limit) : std::vector<library_detail::RetryableRun>{};
}
std::optional<library_detail::RetryableRun> StorageOwner::retryable_run(const std::string& id) const {
  if (!cache_) return std::nullopt;
  try { return cache_->retryable_run(id); }
  catch (const std::invalid_argument&) { return std::nullopt; }
}
bool StorageOwner::discard_retryable(const std::string& id) { return cache_ ? cache_->discard_retryable(id) : false; }
std::size_t StorageOwner::discard_retryable(const std::vector<std::string>& ids) { return cache_ ? cache_->discard_retryable(ids) : 0; }
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
