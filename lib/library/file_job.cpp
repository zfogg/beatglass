#include "beatglass/library/detail/file_job.hpp"
#include "beatglass/library/detail/file_fingerprint.hpp"
#include "beatglass/library/detail/file_snapshot.hpp"
#include "beatglass/library/detail/snapshot_store_job.hpp"
#include "beatglass/library/detail/tempo_analysis_store.hpp"
#include "beatglass/capture/byte_source.hpp"
#include "beatglass/core/causal_replay.hpp"
#include "../capture/file/virtual_io.hpp"
#include <chrono>
#include <vector>
#include <atomic>
#include <iomanip>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
namespace beatglass::library_detail {
namespace {
struct Cancelled {};
// Changes to the shared replay's output semantics require a new suffix.
std::string replay_version(const std::string& engine) { return engine + "/causal-tempo-v1"; }
std::int64_t publication_time() { return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
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
    CacheKey key{before.digest, replay_version(engine), config, "offline"}; std::string blob;
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
    if (published == Publish::error) return {FileJobStatus::error, {}, "cache publication failed or commit outcome unknown", std::make_pair(id, key)};
    return {FileJobStatus::committed, result, {}};
  } catch (const Cancelled&) { return {FileJobStatus::cancelled, {}, {}}; }
    catch (const std::exception& e) { return {FileJobStatus::error, {}, e.what()}; }
}
namespace {
class SnapshotSource final : public ByteSource {
public:
  explicit SnapshotSource(FileSnapshot& snapshot) : snapshot_(snapshot) {}
  std::uint64_t size() const override { return snapshot_.size(); }
  ByteReadResult read_at(std::uint64_t offset, std::span<std::byte> output, std::stop_token stop) override {
    if (stop.stop_requested()) return {ByteReadStatus::cancelled, 0};
    if (output.empty()) return {ByteReadStatus::error, 0};
    if (offset >= size()) return {ByteReadStatus::end, 0};
    auto length = static_cast<std::size_t>((std::min)(static_cast<std::uint64_t>(output.size()), size() - offset));
    auto read = snapshot_.read_at(offset, output.first(length), stop);
    if (read.status == SnapshotStatus::cancelled) return {ByteReadStatus::cancelled, 0};
    if (read.status != SnapshotStatus::ok || read.bytes == 0 || read.bytes > length) return {ByteReadStatus::error, 0};
    return {ByteReadStatus::data, read.bytes};
  }
private:
  FileSnapshot& snapshot_;
};
// The decoder stays borrowed from the snapshot; DSP owns the shared replay policy.
TempoResult replay_snapshot(ByteSource& source, std::uint64_t epoch, std::stop_token stop, const std::function<void(std::uint64_t)>& progress) {
  detail::VirtualFileReader file(source, epoch, stop); constexpr std::size_t hop = 512;
  const auto metadata = file.metadata(); CausalTempoReplay replay;
  std::vector<float> pcm(hop * metadata.channels); TempoResult result{metadata, std::nullopt, 0};
  for (;;) {
    auto read = file.read(pcm, stop);
    if (read.status == FileReadStatus::cancelled) throw Cancelled{};
    if (read.status == FileReadStatus::end) break;
    auto state = replay.process(read.block);
    if (state.beats > std::numeric_limits<unsigned>::max()) throw std::overflow_error("Beat count overflow");
    result.state = state.state; result.beats = static_cast<unsigned>(state.beats); result.bpm = state.bpm;
    if (progress) progress(state.observed_through);
    if (stop.stop_requested()) throw Cancelled{};
  }
  return result;
}
FileJobResult verification_failure(SnapshotStatus status) {
  if (status == SnapshotStatus::cancelled) return {FileJobStatus::cancelled, {}, {}};
  if (status == SnapshotStatus::stale_input || status == SnapshotStatus::unverifiable) return {FileJobStatus::stale_input, {}, {}};
  return {FileJobStatus::error, {}, "source revalidation failed"};
}
}
FileJobResult analyze_snapshot_tempo(SqliteCache& cache, const std::filesystem::path& path,
  const std::string& engine, const std::string& config, std::stop_token stop,
  const std::function<void(std::uint64_t)>& progress) {
  try {
    if (engine.empty() || config.empty()) return {FileJobStatus::error, {}, "empty engine or configuration"};
    auto opened = SnapshotOpen::create(path, stop);
    if (opened.status == SnapshotStatus::error) return {FileJobStatus::error, {}, opened.diagnostic};
    if (opened.status != SnapshotStatus::ok) return verification_failure(opened.status);
    CacheKey key{opened.value->digest(), replay_version(engine), config, "offline"}; std::string blob;
    if (stop.stop_requested()) return {FileJobStatus::cancelled, {}, {}};
    if (cache.find(key, blob)) {
      auto verified = opened.value->verify(stop);
      if (verified != SnapshotStatus::ok) return verification_failure(verified);
      if (stop.stop_requested()) return {FileJobStatus::cancelled, {}, {}};
      return {FileJobStatus::cache_hit, decode(blob), {}};
    }
    const auto id = run_id(); cache.begin(id, key);
    SnapshotSource source(*opened.value);
    auto result = replay_snapshot(source, 0, stop, progress);
    SnapshotStatus verified = SnapshotStatus::error;
    auto published = cache.publish(id, key, encode(result), stop, [&] { verified = opened.value->verify(stop); return verified == SnapshotStatus::ok; });
    if (published == Publish::cancelled) return {FileJobStatus::cancelled, {}, {}};
    if (published == Publish::stale_input) return verification_failure(verified);
    if (published == Publish::error) return {FileJobStatus::error, {}, "cache publication failed or commit outcome unknown", std::make_pair(id, key)};
    return {FileJobStatus::committed, result, {}};
  } catch (const FileOpenCancelled&) { return {FileJobStatus::cancelled, {}, {}}; }
    catch (const Cancelled&) { return {FileJobStatus::cancelled, {}, {}}; }
    catch (const std::exception& e) { return {FileJobStatus::error, {}, e.what()}; }
}

AnalysisIdentity snapshot_tempo_identity(const std::string& content, const std::string& engine, const std::string& configuration) {
  return {content, replay_version(engine), configuration, "offline"};
}
namespace {
class Verifier final : public FileVersionVerifier {
    public:
      explicit Verifier(FileSnapshot& source) : source_(source) {}
      FileVerificationResult verify(std::stop_token token) override {
        auto result = source_.verify(token);
        switch (result) {
          case SnapshotStatus::ok: return {FileVerificationOutcome::match, {}};
          case SnapshotStatus::stale_input: return {FileVerificationOutcome::changed, {}};
          case SnapshotStatus::unverifiable: return {FileVerificationOutcome::unverifiable, {}};
          case SnapshotStatus::cancelled: return {FileVerificationOutcome::cancelled, "verification cancelled"};
          default: return {FileVerificationOutcome::error, "source verification failed"};
        }
      }
    private:
      FileSnapshot& source_;
};
}
static StoredSnapshotJobResult analyze_store_impl(AnalysisStore& store, TempoAnalysisStore* indexed_store, const std::string* index_path, const std::filesystem::path& path,
  const std::string& engine, const std::string& config, std::uint64_t epoch, std::stop_token stop,
  const std::function<void(std::uint64_t)>& progress) {
  try {
    if (!epoch || engine.empty() || config.empty()) return {FileJobStatus::error, {}, "invalid epoch, engine or configuration", {}};
    if (index_path && std::filesystem::absolute(std::filesystem::u8path(*index_path)).lexically_normal() != std::filesystem::absolute(path).lexically_normal()) return {FileJobStatus::error, {}, "index path mismatch", {}};
#ifdef _WIN32
    const auto policy = indexed_store ? SnapshotSourcePolicy::deny_writes : SnapshotSourcePolicy::detect_changes;
#else
    const auto policy = SnapshotSourcePolicy::detect_changes;
#endif
    auto opened = SnapshotOpen::create(path, stop, {}, policy);
    if (opened.status == SnapshotStatus::cancelled) return {FileJobStatus::cancelled, {}, {}, {}};
    if (opened.status == SnapshotStatus::stale_input || opened.status == SnapshotStatus::unverifiable) return {FileJobStatus::stale_input, {}, opened.diagnostic, {}};
    if (opened.status != SnapshotStatus::ok) return {FileJobStatus::error, {}, opened.diagnostic, {}};
    auto modified = opened.value->modified_unix_ns();
    if (!modified) return {FileJobStatus::stale_input, {}, "original modification time cannot be represented", {}};
    FileVersion file{std::filesystem::absolute(path), opened.value->size(), *modified, opened.value->digest()};
    AnalysisIdentity identity = snapshot_tempo_identity(opened.value->digest(), engine, config);
    Verifier verifier(*opened.value);
    auto found = store.find(file, identity, stop);
    if (found.outcome == FindOutcome::cancelled) return {FileJobStatus::cancelled, {}, {}, {}};
    if (found.outcome == FindOutcome::error) return {FileJobStatus::error, {}, found.diagnostic, {}};
    if (found.outcome == FindOutcome::hit) {
      if (!found.result) return {FileJobStatus::error, {}, "store returned hit without value", {}};
      if (indexed_store) {
        auto associated = indexed_store->associate(file, identity, *index_path, verifier, stop);
        if (associated.outcome == PublishOutcome::cancelled) return {FileJobStatus::cancelled, {}, associated.diagnostic, {}};
        if (associated.outcome == PublishOutcome::stale_input) return {FileJobStatus::stale_input, {}, associated.diagnostic, {}};
        if (associated.outcome != PublishOutcome::committed) return {FileJobStatus::error, {}, associated.diagnostic, {}};
      }
      auto verified = indexed_store ? SnapshotStatus::ok : opened.value->verify(stop);
      if (verified == SnapshotStatus::cancelled || (!indexed_store && stop.stop_requested())) return {FileJobStatus::cancelled, {}, {}, {}};
      if (verified == SnapshotStatus::stale_input || verified == SnapshotStatus::unverifiable) return {FileJobStatus::stale_input, {}, {}, {}};
      if (verified != SnapshotStatus::ok) return {FileJobStatus::error, {}, "source revalidation failed", {}};
      auto& snapshot = found.result->result; const auto old_epoch = snapshot.observed_through.epoch;
      for (auto& beat : snapshot.beats) { if (beat.time.epoch != old_epoch) return {FileJobStatus::error, {}, "cached beat epoch mismatch", {}}; beat.time.epoch = epoch; }
      for (auto& chord : snapshot.chords) { if (chord.start.epoch != old_epoch || chord.end.epoch != old_epoch) return {FileJobStatus::error, {}, "cached chord epoch mismatch", {}}; chord.start.epoch = epoch; chord.end.epoch = epoch; }
      snapshot.observed_through.epoch = epoch; snapshot.publication_monotonic_ns = publication_time();
      return {FileJobStatus::cache_hit, std::move(found.result), {}, {}};
    }
    if (found.outcome != FindOutcome::miss) return {FileJobStatus::error, {}, "unknown store lookup outcome", {}};
    SnapshotSource source(*opened.value); std::uint64_t observed = 0;
    auto tempo = replay_snapshot(source, epoch, stop, [&](auto frames) { observed = frames; if (progress) progress(frames); });
    CachedAnalysis value; value.identity = identity; value.result.state = tempo.state; value.result.bpm = tempo.bpm;
    value.result.observed_through = {epoch, observed}; value.result.publication_monotonic_ns = publication_time(); value.result.estimator_version = identity.engine_version; value.result.configuration_id = config;

    auto id = run_id(); auto published = indexed_store ? indexed_store->publish_indexed(file, value, id, *index_path, verifier, stop) : store.publish(file, value, id, verifier, stop);
    if (published.outcome == PublishOutcome::committed) { value.result.publication_monotonic_ns = publication_time(); return {FileJobStatus::committed, value, {}, {}}; }
    if (published.outcome == PublishOutcome::cancelled) return {FileJobStatus::cancelled, {}, {}, {}};
    if (published.outcome == PublishOutcome::stale_input) return {FileJobStatus::stale_input, {}, published.diagnostic, {}};
    return {FileJobStatus::error, {}, published.diagnostic, id};
  } catch (const FileOpenCancelled&) { return {FileJobStatus::cancelled, {}, {}, {}}; }
    catch (const Cancelled&) { return {FileJobStatus::cancelled, {}, {}, {}}; }
    catch (const std::exception& e) { return {FileJobStatus::error, {}, e.what(), {}}; }
}

StoredSnapshotJobResult analyze_snapshot_to_store(AnalysisStore& store, const std::filesystem::path& path, const std::string& engine, const std::string& config, std::uint64_t epoch, std::stop_token stop, const std::function<void(std::uint64_t)>& progress) {
  return analyze_store_impl(store, nullptr, nullptr, path, engine, config, epoch, stop, progress);
}
StoredSnapshotJobResult analyze_indexed_snapshot_to_store(TempoAnalysisStore& store, const std::string& index_path, const std::filesystem::path& path, const std::string& engine, const std::string& config, std::uint64_t epoch, std::stop_token stop, const std::function<void(std::uint64_t)>& progress) {
  return analyze_store_impl(store, &store, &index_path, path, engine, config, epoch, stop, progress);
}

}
