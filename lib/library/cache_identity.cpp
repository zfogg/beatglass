#include "beatglass/library/detail/cache_identity.hpp"
#include <cstdint>
#include <limits>
#include <system_error>
namespace beatglass::library_detail {
CacheIdentityResult cache_identity_from_file(const std::filesystem::path& path,
                                             std::string engine,
                                             std::string config,
                                             std::string mode,
                                             std::stop_token stop) {
  CacheIdentityResult out;
  const auto fingerprint = fingerprint_file(path, stop);
  out.status = fingerprint.status;
  out.diagnostic = fingerprint_status_diagnostic(fingerprint.status, fingerprint.diagnostic);
  if (fingerprint.status != FingerprintStatus::ok) return out;
  out.key = CacheKey{fingerprint.digest, std::move(engine), std::move(config), std::move(mode)};
  return out;
}

FingerprintStatus refresh_collection_file(SqliteCache& cache, const std::filesystem::path& path,
                                          std::stop_token stop, std::string* diagnostic) {
  const auto fingerprint = fingerprint_file(path, stop);
  if (diagnostic) *diagnostic = fingerprint_status_diagnostic(fingerprint.status, fingerprint.diagnostic);
  if (fingerprint.status != FingerprintStatus::ok) return fingerprint.status;
  const auto name = path.filename().string();
  // Apple exposes file_time_type::rep as a wider integer than SQLite's
  // signed 64-bit metadata column. Clamp at the storage boundary so the
  // identity remains deterministic without relying on implementation-defined
  // narrowing.
  const auto modified = fingerprint.modified.time_since_epoch().count();
  const auto modified_i64 = modified > std::numeric_limits<std::int64_t>::max()
      ? std::numeric_limits<std::int64_t>::max()
      : modified < std::numeric_limits<std::int64_t>::min()
          ? std::numeric_limits<std::int64_t>::min()
          : static_cast<std::int64_t>(modified);
  cache.put_entry({path.string(), name, "audio", fingerprint.digest, fingerprint.size,
                   modified_i64});
  return FingerprintStatus::ok;
}

FingerprintStatus revalidate_file_identity(const std::filesystem::path& path,
                                           const std::string& expected_content,
                                           std::stop_token stop, std::string* diagnostic) {
  const auto fingerprint = fingerprint_file(path, stop);
  if (diagnostic) *diagnostic = fingerprint.diagnostic;
  if (fingerprint.status != FingerprintStatus::ok) return fingerprint.status;
  if (fingerprint.digest != expected_content) {
    if (diagnostic) *diagnostic = "content identity changed";
    return FingerprintStatus::changed;
  }
  return FingerprintStatus::ok;
}

FingerprintStatus invalidate_if_stale(SqliteCache& cache, const CacheKey& key,
                                      const std::filesystem::path& path,
                                      std::stop_token stop, std::string* diagnostic) {
  const auto status = revalidate_file_identity(path, key.content, stop, diagnostic);
  if (fingerprint_status_requires_refresh(status)) cache.invalidate(key);
  return status;
}

Publish publish_file_result(SqliteCache& cache, const std::string& run_id,
                            const CacheKey& key, const std::filesystem::path& path,
                            const std::string& result, std::stop_token stop) {
  if (result.empty()) return Publish::error;
  const auto initial = revalidate_file_identity(path, key.content, stop);
  if (initial == FingerprintStatus::cancelled) return Publish::cancelled;
  if (fingerprint_status_requires_refresh(initial)) { cache.invalidate(key); return Publish::stale_input; }
  if (initial != FingerprintStatus::ok) return Publish::stale_input;
  if (stop.stop_requested()) return Publish::cancelled;
  cache.begin(run_id, key);
  return cache.publish(run_id, key, result, stop, [&] {
    // Cancellation only abandons this in-flight publication. It is not proof
    // that the existing completed cache entry is stale, so preserve it.
    if (stop.stop_requested()) return false;
    const auto status = revalidate_file_identity(path, key.content, stop);
    if (fingerprint_status_requires_refresh(status)) cache.invalidate(key);
    return status == FingerprintStatus::ok;
  });
}

FingerprintStatus invalidate_collection_if_stale(SqliteCache& cache, const std::string& path,
                                                 const std::string& expected_content,
                                                 std::stop_token stop, std::string* diagnostic) {
  const auto status = revalidate_file_identity(path, expected_content, stop, diagnostic);
  // An unreadable path is not proof that the catalog entry is stale. Keep it
  // available with its diagnostic so a transient permission/device failure can
  // be retried without losing the user's catalog row.
  if (fingerprint_status_requires_refresh(status)) {
    cache.remove_entry(path);
    if (diagnostic) *diagnostic = fingerprint_status_diagnostic(status, "content identity changed; catalog entry removed");
  }
  return status;
}

bool prune_missing_collection_entry(SqliteCache& cache, const std::filesystem::path& path,
                                    std::string* diagnostic) {
  std::error_code ec;
  const bool exists = std::filesystem::exists(path, ec);
  if (ec) { if (diagnostic) *diagnostic = ec.message(); return false; }
  if (exists) return false;
  const bool removed = cache.remove_entry(path.string());
  if (removed && diagnostic) *diagnostic = "source path missing; catalog entry removed";
  return removed;
}
}
