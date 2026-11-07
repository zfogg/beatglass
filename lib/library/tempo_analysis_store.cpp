#include "beatglass/library/detail/tempo_analysis_store.hpp"
#include "beatglass/library/detail/sqlite_cache.hpp"
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
namespace beatglass::library_detail {
namespace {
CacheKey key_of(const AnalysisIdentity& id) { return {id.content_hash, "snapshot-store-v1:" + id.engine_version, id.configuration_id, id.mode}; }
bool valid_identity(const FileVersion& file, const AnalysisIdentity& id) {
  return !file.content_hash.empty() && file.content_hash.size() <= 512 && file.content_hash == id.content_hash && !id.engine_version.empty() && id.engine_version.size() <= 4096 && !id.configuration_id.empty() && id.configuration_id.size() <= 4096 && (id.mode == "offline" || id.mode == "live");
}
std::string encode(const Snapshot& s) {
  if (!s.tempo_candidates.empty() || !s.key_candidates.empty() || !s.beats.empty() || !s.chords.empty()) throw std::invalid_argument("tempo-only store cannot serialize event or candidate collections");
  if (s.estimator_version.size() > 4096 || s.configuration_id.size() > 4096) throw std::invalid_argument("snapshot identity too large");
  if (s.bpm && !std::isfinite(*s.bpm)) throw std::invalid_argument("invalid BPM");
  if (static_cast<int>(s.state) < 0 || static_cast<int>(s.state) > static_cast<int>(TrackingState::error)) throw std::invalid_argument("invalid tracking state");
  std::ostringstream out; out.imbue(std::locale::classic());
  out << "snapshot-tempo-v1 " << static_cast<int>(s.state) << ' ' << s.observed_through.epoch << ' ' << s.observed_through.frame << ' '
      << s.publication_monotonic_ns << ' ' << s.analysis_delay_frames << ' ' << s.bpm.has_value() << ' '
      << std::setprecision(std::numeric_limits<float>::max_digits10) << s.bpm.value_or(0) << ' '
      << std::quoted(s.estimator_version) << ' ' << std::quoted(s.configuration_id);
  return out.str();
}
Snapshot decode(const std::string& data) {
  if (data.size() > 1024 * 1024) throw std::runtime_error("cached result too large");
  std::istringstream in(data); in.imbue(std::locale::classic());
  Snapshot s; std::string version; int state = 0; bool present = false; float bpm = 0;
  if (!(in >> version >> state >> s.observed_through.epoch >> s.observed_through.frame >> s.publication_monotonic_ns >> s.analysis_delay_frames >> present >> bpm >> std::quoted(s.estimator_version) >> std::quoted(s.configuration_id)) || version != "snapshot-tempo-v1" || state < 0 || state > static_cast<int>(TrackingState::error) || !std::isfinite(bpm)) throw std::runtime_error("invalid cached tempo snapshot");
  in >> std::ws; if (!in.eof()) throw std::runtime_error("trailing cached snapshot data");
  s.state = static_cast<TrackingState>(state); if (present) s.bpm = bpm; return s;
}
}
struct TempoAnalysisStore::Impl { explicit Impl(const std::string& path) : cache(path) {} SqliteCache cache; };
TempoAnalysisStore::TempoAnalysisStore(const std::string& path) : impl_(std::make_unique<Impl>(path)) {}
TempoAnalysisStore::~TempoAnalysisStore() = default;
void TempoAnalysisStore::put_entry(const CollectionEntry& entry) { impl_->cache.put_entry(entry); }
bool TempoAnalysisStore::remove_entry(const std::string& path) { return impl_->cache.remove_entry(path); }
std::optional<CollectionEntry> TempoAnalysisStore::entry(const std::string& path, std::stop_token stop) { return impl_->cache.entry(path, stop); }
std::vector<CollectionEntry> TempoAnalysisStore::entries(const CollectionQuery& query, std::stop_token stop) { return impl_->cache.entries(query, stop); }
SqliteCache& TempoAnalysisStore::offline_cache() { return impl_->cache; }
FindResult TempoAnalysisStore::find(const FileVersion& file, const AnalysisIdentity& id, std::stop_token stop) {
  if (stop.stop_requested()) return {FindOutcome::cancelled, {}, {}};
  if (!valid_identity(file, id)) return {FindOutcome::error, {}, "invalid content or analysis identity"};
  try {
    std::string blob; bool hit = impl_->cache.find(key_of(id), blob);
    if (stop.stop_requested()) return {FindOutcome::cancelled, {}, {}};
    if (!hit) return {FindOutcome::miss, {}, {}};
    auto value = decode(blob);
    if (value.estimator_version != id.engine_version || value.configuration_id != id.configuration_id) throw std::runtime_error("cached result identity mismatch");
    return {FindOutcome::hit, CachedAnalysis{id, std::move(value)}, {}};
  } catch (const std::exception& e) { return {FindOutcome::error, {}, e.what()}; }
}
PublishResult TempoAnalysisStore::publish(const FileVersion& file, const CachedAnalysis& value, const std::string& run, FileVersionVerifier& verifier, std::stop_token stop) {
  return publish_impl(file, value, run, nullptr, verifier, stop);
}
PublishResult TempoAnalysisStore::publish_indexed(const FileVersion& file, const CachedAnalysis& value, const std::string& run, const std::string& path, FileVersionVerifier& verifier, std::stop_token stop) {
  return publish_impl(file, value, run, &path, verifier, stop);
}
PublishResult TempoAnalysisStore::publish_impl(const FileVersion& file, const CachedAnalysis& value, const std::string& run, const std::string* path, FileVersionVerifier& verifier, std::stop_token stop) {
  if (stop.stop_requested()) return {PublishOutcome::cancelled, {}};
  if (run.empty() || run.size() > 4096 || !valid_identity(file, value.identity) || value.result.estimator_version != value.identity.engine_version || value.result.configuration_id != value.identity.configuration_id) return {PublishOutcome::error, "invalid content, run or analysis identity"};
  try {
    auto blob = encode(value.result); if (blob.size() > 1024 * 1024) return {PublishOutcome::error, "result too large"};
    auto key = key_of(value.identity); impl_->cache.begin(run, key);
    FileVerificationResult verification; bool verified = false;
    auto revalidate = [&] { verification = verifier.verify(stop); verified = true; return verification.outcome == FileVerificationOutcome::match; };
    auto status = path ? impl_->cache.publish_indexed(run, key, blob, *path, stop, revalidate) : impl_->cache.publish(run, key, blob, stop, revalidate);
    if (status == Publish::committed) return {PublishOutcome::committed, {}};
    if (status == Publish::cancelled || (status == Publish::stale_input && stop.stop_requested())) return {PublishOutcome::cancelled, {}};
    if (status == Publish::stale_input && verified && verification.outcome == FileVerificationOutcome::cancelled) return {PublishOutcome::cancelled, verification.diagnostic};
    if (status == Publish::stale_input && verified && (verification.outcome == FileVerificationOutcome::changed || verification.outcome == FileVerificationOutcome::unverifiable)) return {PublishOutcome::stale_input, verification.diagnostic};
    if (status == Publish::stale_input && verified) return {PublishOutcome::error, verification.diagnostic.empty() ? "input verifier failed" : verification.diagnostic};
    return {PublishOutcome::error, "storage failure; commit outcome may be unknown; recover by run ID"};
  } catch (const std::exception& e) { return {PublishOutcome::error, e.what()}; }
}
PublishResult TempoAnalysisStore::associate(const FileVersion& file, const AnalysisIdentity& identity, const std::string& path, FileVersionVerifier& verifier, std::stop_token stop) {
  if (stop.stop_requested()) return {PublishOutcome::cancelled, {}};
  if (!valid_identity(file, identity) || path.empty()) return {PublishOutcome::error, "invalid identity or path"};
  FileVerificationResult verification; bool verified = false;
  auto status = impl_->cache.associate(key_of(identity), path, stop, [&] { verification = verifier.verify(stop); verified = true; return verification.outcome == FileVerificationOutcome::match; });
  if (status == Publish::committed) return {PublishOutcome::committed, {}};
  if (status == Publish::cancelled || (status == Publish::stale_input && verified && verification.outcome == FileVerificationOutcome::cancelled)) return {PublishOutcome::cancelled, {}};
  if (status == Publish::stale_input && verified && (verification.outcome == FileVerificationOutcome::changed || verification.outcome == FileVerificationOutcome::unverifiable)) return {PublishOutcome::stale_input, verification.diagnostic};
  return {PublishOutcome::error, "association failed or verifier error"};
}
RecoveryResult TempoAnalysisStore::recover(const std::string& run) {
  if (run.empty()) return {RecoveryOutcome::error, "empty run ID"};
  try { return {impl_->cache.committed_run(run) ? RecoveryOutcome::committed : RecoveryOutcome::not_found, {}}; }
  catch (const std::exception& e) { return {RecoveryOutcome::error, e.what()}; }
}
}
