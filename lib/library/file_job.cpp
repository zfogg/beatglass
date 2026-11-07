#include "beatglass/library/detail/file_job.hpp"
#include "beatglass/library/detail/file_fingerprint.hpp"
#include <atomic>
#include <iomanip>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
namespace beatglass::library_detail {
namespace {
struct Cancelled {};
std::string run_id() {
  static std::atomic<unsigned long long> counter{0};
  std::random_device random;
  std::ostringstream s; s << std::hex << random() << '-' << random() << '-' << counter.fetch_add(1); return s.str();
}
std::string encode(const TempoResult& r) {
  std::ostringstream s; s.imbue(std::locale::classic());
  s << "tempo-v1 " << r.metadata.sample_rate << ' ' << r.metadata.channels << ' ' << r.metadata.frames << ' '
    << std::quoted(r.metadata.format) << ' ' << r.bpm.has_value() << ' '
    << std::setprecision(std::numeric_limits<float>::max_digits10) << r.bpm.value_or(0) << ' '
    << r.beats << ' ' << static_cast<int>(r.state);
  return s.str();
}
TempoResult decode(const std::string& data) {
  std::istringstream s(data); s.imbue(std::locale::classic());
  std::string version; TempoResult r; bool has = false; float bpm = 0; int state = 0;
  if (!(s >> version >> r.metadata.sample_rate >> r.metadata.channels >> r.metadata.frames >> std::quoted(r.metadata.format) >> has >> bpm >> r.beats >> state)
      || version != "tempo-v1" || state < 0 || state > static_cast<int>(TrackingState::error)) throw std::runtime_error("invalid cached tempo result");
  s >> std::ws; if (!s.eof()) throw std::runtime_error("trailing cached tempo result");
  r.state = static_cast<TrackingState>(state); if (has) r.bpm = bpm; return r;
}
}
FileJobResult analyze_cached_tempo(SqliteCache& cache, const std::filesystem::path& path,
  const std::string& engine, const std::string& config, std::stop_token stop,
  const std::function<void(std::uint64_t)>& progress) {
  try {
    if (engine.empty() || config.empty()) return {FileJobStatus::error, {}, "empty engine or configuration"};
    auto before = fingerprint_file(path, stop);
    if (before.status == FingerprintStatus::cancelled) return {FileJobStatus::cancelled, {}, {}};
    if (before.status == FingerprintStatus::changed) return {FileJobStatus::stale_input, {}, {}};
    if (before.status != FingerprintStatus::ok) return {FileJobStatus::error, {}, before.diagnostic};
    CacheKey key{before.digest, engine, config, "offline"}; std::string blob;
    if (stop.stop_requested()) return {FileJobStatus::cancelled, {}, {}};
    if (cache.find(key, blob)) return {FileJobStatus::cache_hit, decode(blob), {}};
    const auto id = run_id(); cache.begin(id, key);
    if (stop.stop_requested()) return {FileJobStatus::cancelled, {}, {}};
    auto result = replay_file(path, [&](std::uint64_t frames, const TempoResult&) {
      if (progress) progress(frames);
      if (stop.stop_requested()) throw Cancelled{};
    });
    bool cancelled = false;
    bool validation_error = false;
    std::string validation_diagnostic;
    const auto published = cache.publish(id, key, encode(result), stop, [&] {
      const auto after = fingerprint_file(path, stop);
      cancelled = after.status == FingerprintStatus::cancelled;
      validation_error = after.status == FingerprintStatus::error;
      validation_diagnostic = after.diagnostic;
      return after.status == FingerprintStatus::ok && before.size == after.size && before.modified == after.modified && before.digest == after.digest;
    });
    if (cancelled || published == Publish::cancelled) return {FileJobStatus::cancelled, {}, {}};
    if (validation_error) return {FileJobStatus::error, {}, validation_diagnostic};
    if (published == Publish::stale_input) return {FileJobStatus::stale_input, {}, {}};
    if (published == Publish::error) return {FileJobStatus::error, {}, "cache publication failed or commit outcome unknown"};
    return {FileJobStatus::committed, result, {}};
  } catch (const Cancelled&) { return {FileJobStatus::cancelled, {}, {}}; }
    catch (const std::exception& e) { return {FileJobStatus::error, {}, e.what()}; }
}
}
