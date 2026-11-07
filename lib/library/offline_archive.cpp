#include "beatglass/library/detail/offline_archive.hpp"
#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
namespace beatglass::library_detail {
namespace {
constexpr OfflineLimits limits{100000, 100000, 64, 4096};
struct ArchiveCancelled {};
void check_stop(std::stop_token stop) { if (stop.stop_requested()) throw ArchiveCancelled{}; }
constexpr std::size_t max_blob = 8 * 1024 * 1024;
CacheKey cache_key(const OfflineKey& key) {
  if (key.content_hash.empty() || key.content_hash.size() > 512 || key.engine_version.empty() || key.engine_version.size() > 4096 || key.configuration_id.empty() || key.configuration_id.size() > 4096 || key.algorithms.empty() || key.algorithms.size() > limits.max_algorithms || (key.mode != OfflineMode::causal && key.mode != OfflineMode::refined)) throw std::invalid_argument("invalid offline identity");
  std::ostringstream engine; engine << "offline-archive-v1:" << key.engine_version.size() << ':' << key.engine_version;
  std::string previous;
  for (const auto& a : key.algorithms) {
    if (a.algorithm.empty() || a.algorithm.size() > 4096 || a.version.empty() || a.version.size() > 4096 || (!previous.empty() && previous >= a.algorithm)) throw std::invalid_argument("algorithm identities must be sorted and unique");
    engine << ':' << a.algorithm.size() << ':' << a.algorithm << ':' << a.version.size() << ':' << a.version; previous = a.algorithm;
  }
  return {key.content_hash, engine.str(), key.configuration_id, key.mode == OfflineMode::causal ? "offline-causal" : "offline-refined"};
}
bool matches(const OfflineKey& key, const OfflineAnalysis& v) {
  if (key.engine_version != v.engine_version || key.configuration_id != v.configuration_id || key.mode != v.mode || key.algorithms.size() != v.algorithms.size()) return false;
  for (std::size_t i = 0; i < key.algorithms.size(); ++i) if (key.algorithms[i].algorithm != v.algorithms[i].algorithm || key.algorithms[i].version != v.algorithms[i].version) return false;
  return true;
}
void estimate(std::ostream& out, const std::optional<OfflineEstimate>& e) {
  out << ' ' << bool(e); if (e) out << ' ' << std::quoted(e->label) << ' ' << bool(e->strength) << ' ' << e->strength.value_or(0);
}
std::optional<OfflineEstimate> estimate(std::istream& in) {
  bool present = false, strength = false; OfflineEstimate e; float value = 0;
  if (!(in >> present)) throw std::runtime_error("invalid estimate"); if (!present) return {};
  if (!(in >> std::quoted(e.label) >> strength >> value)) throw std::runtime_error("invalid estimate"); if (strength) e.strength = value; return e;
}
void bound(std::ostream& out) { if (out.tellp() < 0 || static_cast<std::uint64_t>(out.tellp()) > max_blob) throw std::length_error("offline result too large"); }
std::string encode(const OfflineAnalysis& v, std::stop_token stop) {
  check_stop(stop);
  std::ostringstream o; o.imbue(std::locale::classic()); o << std::setprecision(std::numeric_limits<float>::max_digits10);
  o << "offline-v1 " << v.epoch << ' ' << v.sample_rate << ' ' << v.observed_through << ' ' << v.analyzed_through << ' ' << static_cast<int>(v.mode) << ' ' << std::quoted(v.engine_version) << ' ' << std::quoted(v.configuration_id) << ' ' << v.algorithms.size();
  for (const auto& a : v.algorithms) { o << ' ' << std::quoted(a.algorithm) << ' ' << std::quoted(a.version); bound(o); check_stop(stop); }
  o << ' ' << bool(v.bpm) << ' ' << v.bpm.value_or(0); estimate(o, v.validated_track_key); o << ' ' << v.beats.size();
  for (const auto& b : v.beats) { o << ' ' << b.frame << ' ' << b.observed_through << ' ' << bool(b.strength) << ' ' << b.strength.value_or(0); bound(o); check_stop(stop); }
  o << ' ' << v.tonal_windows.size();
  for (const auto& w : v.tonal_windows) { o << ' ' << w.start_frame << ' ' << w.end_frame << ' ' << bool(w.rolling_key_from) << ' ' << w.rolling_key_from.value_or(0); estimate(o, w.rolling_key); estimate(o, w.local_chord); bound(o); check_stop(stop); }
  auto text = o.str(); if (text.size() > max_blob) throw std::length_error("offline result too large"); return text;
}
OfflineAnalysis decode(const std::string& blob, std::stop_token stop) {
  check_stop(stop);
  if (blob.size() > max_blob) throw std::runtime_error("offline result too large"); std::istringstream in(blob); in.imbue(std::locale::classic());
  OfflineAnalysis v; std::string version; int mode = 0; std::size_t count = 0; bool present = false; float strength = 0;
  if (!(in >> version >> v.epoch >> v.sample_rate >> v.observed_through >> v.analyzed_through >> mode >> std::quoted(v.engine_version) >> std::quoted(v.configuration_id) >> count) || version != "offline-v1" || mode < 0 || mode > 1 || count > limits.max_algorithms) throw std::runtime_error("invalid offline header");
  v.mode = static_cast<OfflineMode>(mode); v.algorithms.resize(count);
  for (auto& a : v.algorithms) { check_stop(stop); if (!(in >> std::quoted(a.algorithm) >> std::quoted(a.version))) throw std::runtime_error("invalid algorithms"); }
  if (!(in >> present >> strength)) throw std::runtime_error("invalid tempo"); if (present) v.bpm = strength; v.validated_track_key = estimate(in);
  if (!(in >> count) || count > limits.max_beats) throw std::runtime_error("invalid beats"); v.beats.resize(count);
  for (auto& b : v.beats) { check_stop(stop); if (!(in >> b.frame >> b.observed_through >> present >> strength)) throw std::runtime_error("invalid beat"); if (present) b.strength = strength; }
  if (!(in >> count) || count > limits.max_tonal_windows) throw std::runtime_error("invalid windows"); v.tonal_windows.resize(count);
  for (auto& w : v.tonal_windows) { check_stop(stop); std::uint64_t from = 0; if (!(in >> w.start_frame >> w.end_frame >> present >> from)) throw std::runtime_error("invalid window"); if (present) w.rolling_key_from = from; w.rolling_key = estimate(in); w.local_chord = estimate(in); }
  in >> std::ws; if (!in.eof() || validate_offline_analysis(v, limits) != OfflineValidation::valid) throw std::runtime_error("invalid offline result"); return v;
}
}
PublishResult OfflineArchive::publish(const FileVersion& source, const OfflineKey& key, const OfflineAnalysis& value, const std::string& run, const std::string& path, FileVersionVerifier& verifier, std::stop_token stop) {
  if (stop.stop_requested()) return {PublishOutcome::cancelled, {}};
  try {
    if (source.content_hash != key.content_hash || source.path.empty() || path.empty() || std::filesystem::absolute(std::filesystem::u8path(path)).lexically_normal() != std::filesystem::absolute(source.path).lexically_normal() || run.empty() || run.size() > 4096 || !matches(key, value) || validate_offline_analysis(value, limits) != OfflineValidation::valid) return {PublishOutcome::error, "invalid offline value or identity"};
    auto k = cache_key(key); auto blob = encode(value, stop); cache_.begin(run, k); FileVerificationResult verification; bool called = false;
    auto status = cache_.publish_indexed(run, k, blob, path, stop, [&] { called = true; verification = verifier.verify(stop); return verification.outcome == FileVerificationOutcome::match; });
    if (status == Publish::committed) return {PublishOutcome::committed, {}};
    if (status == Publish::cancelled || (status == Publish::stale_input && called && verification.outcome == FileVerificationOutcome::cancelled)) return {PublishOutcome::cancelled, {}};
    if (status == Publish::stale_input && called && (verification.outcome == FileVerificationOutcome::changed || verification.outcome == FileVerificationOutcome::unverifiable)) return {PublishOutcome::stale_input, verification.diagnostic};
    return {PublishOutcome::error, "offline publication failed; recover run ID after reopening"};
  } catch (const ArchiveCancelled&) { return {PublishOutcome::cancelled, {}}; } catch (const std::exception& e) { return {PublishOutcome::error, e.what()}; }
}
OfflineReadResult OfflineArchive::page(const FileVersion& source, const OfflineKey& key, const std::string& requested_run, std::size_t bo, std::size_t bc, std::size_t wo, std::size_t wc, std::uint64_t epoch, FileVersionVerifier& verifier, std::stop_token stop) {
  if (stop.stop_requested()) return {OfflineReadStatus::cancelled, {}, {}};
  try {
    if (!epoch || bc > 200 || wc > 200 || source.content_hash != key.content_hash) return {OfflineReadStatus::error, {}, "invalid offline page request"};
    auto k = cache_key(key); auto active = cache_.active_run(k);
    if (!active) return {OfflineReadStatus::miss, {}, {}};
    if (!requested_run.empty() && requested_run != *active) return {OfflineReadStatus::changed, {}, "active run was replaced"};
    auto run = active;
    std::string blob; if (!cache_.find_run(*run, k, blob)) return {OfflineReadStatus::miss, {}, {}};
    if (stop.stop_requested()) return {OfflineReadStatus::cancelled, {}, {}};
    auto v = decode(blob, stop); if (!matches(key, v)) return {OfflineReadStatus::error, {}, "offline identity mismatch"};
    OfflinePage out; out.run_id = *run; out.total_beats = v.beats.size(); out.total_windows = v.tonal_windows.size();
    if (bo > out.total_beats || wo > out.total_windows) return {OfflineReadStatus::error, {}, "offline page offset"};
    out.next_beat = bo + (std::min)(bc, out.total_beats - bo); out.next_window = wo + (std::min)(wc, out.total_windows - wo);
    out.beats.assign(v.beats.begin() + bo, v.beats.begin() + out.next_beat); out.windows.assign(v.tonal_windows.begin() + wo, v.tonal_windows.begin() + out.next_window);
    std::vector<OfflineBeat>().swap(v.beats); std::vector<OfflineTonalWindow>().swap(v.tonal_windows); v.epoch = epoch; out.header = std::move(v);
    auto check = verifier.verify(stop);
    if (check.outcome == FileVerificationOutcome::cancelled || stop.stop_requested()) return {OfflineReadStatus::cancelled, {}, {}};
    if (check.outcome == FileVerificationOutcome::changed || check.outcome == FileVerificationOutcome::unverifiable) return {OfflineReadStatus::changed, {}, check.diagnostic};
    if (check.outcome != FileVerificationOutcome::match) return {OfflineReadStatus::error, {}, check.diagnostic};
    return {OfflineReadStatus::hit, std::move(out), {}};
  } catch (const ArchiveCancelled&) { return {OfflineReadStatus::cancelled, {}, {}}; } catch (const std::exception& e) { return {OfflineReadStatus::error, {}, e.what()}; }
}
}
