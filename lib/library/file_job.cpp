#include "beatglass/library/detail/file_job.hpp"
#include "beatglass/library/detail/file_fingerprint.hpp"
#include "beatglass/library/detail/file_snapshot.hpp"
#include "beatglass/library/detail/snapshot_store_job.hpp"
#include "beatglass/capture/byte_source.hpp"
#include "beatglass/core/tempo_tracker.hpp"
#include "beatglass/core/channel_reduction.hpp"
#include "beatglass/core/stream_continuity.hpp"
#include "beatglass/core/tracking_machine.hpp"
#include "../capture/file/virtual_io.hpp"
#include <algorithm>
#include <cmath>
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
// Temporary integration adapter. DSP should own a shared replay pipeline once
// its virtual reader interface is accepted; this mirrors the current tempo slice.
TempoResult replay_snapshot(ByteSource& source, std::stop_token stop, const std::function<void(std::uint64_t)>& progress) {
  detail::VirtualFileReader file(source, 0, stop); constexpr std::size_t hop = 512;
  const auto metadata = file.metadata(); TempoTracker tracker(metadata.sample_rate);
  std::uint64_t position = 0, silent_frames = 0; StreamContinuity continuity; TrackingMachine lifecycle;
  if (!lifecycle.start(0)) throw std::runtime_error("Could not start tracking");
  std::vector<float> pcm(hop * metadata.channels), mono(hop); TempoResult result{metadata, std::nullopt, 0};
  for (;;) {
    auto read = file.read(pcm, stop);
    if (read.status == FileReadStatus::cancelled) throw Cancelled{};
    if (read.status == FileReadStatus::end) break;
    const auto& block = read.block; auto n = block.samples.size() / metadata.channels;
    auto transition = continuity.observe(block);
    if (transition == StreamTransition::invalid) throw std::runtime_error("Invalid decoded audio block");
    mono.resize(n);
    if (!reduce_loudest_channel(block.samples, metadata.channels, mono)) throw std::runtime_error("Audio contains invalid samples");
    bool active = std::any_of(mono.begin(), mono.end(), [](float sample) { return std::abs(sample) >= 0.0001f; });
    bool reset = transition == StreamTransition::reacquire;
    if (reset) lifecycle.discontinuity(block.epoch);
    if (active) { silent_frames = 0; lifecycle.signal(); }
    else {
      silent_frames = (std::min)(static_cast<std::uint64_t>(metadata.sample_rate), silent_frames + mono.size());
      if (silent_frames >= metadata.sample_rate && lifecycle.state() != TrackingState::silent) { lifecycle.silence(); reset = true; }
    }
    tracker.process(mono, block.epoch, position, reset); position += mono.size();
    auto current = tracker.estimate();
    if (current.bpm && lifecycle.state() != TrackingState::silent) lifecycle.accept(lifecycle.token());
    result.state = lifecycle.state(); result.beats = static_cast<unsigned>(current.beats);
    result.bpm = result.state == TrackingState::tracking ? current.bpm : std::nullopt;
    if (progress) progress(position);
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
    CacheKey key{opened.value->digest(), engine, config, "offline"}; std::string blob;
    if (stop.stop_requested()) return {FileJobStatus::cancelled, {}, {}};
    if (cache.find(key, blob)) {
      auto verified = opened.value->verify(stop);
      if (verified != SnapshotStatus::ok) return verification_failure(verified);
      if (stop.stop_requested()) return {FileJobStatus::cancelled, {}, {}};
      return {FileJobStatus::cache_hit, decode(blob), {}};
    }
    const auto id = run_id(); cache.begin(id, key);
    SnapshotSource source(*opened.value);
    auto result = replay_snapshot(source, stop, progress);
    SnapshotStatus verified = SnapshotStatus::error;
    auto published = cache.publish(id, key, encode(result), stop, [&] { verified = opened.value->verify(stop); return verified == SnapshotStatus::ok; });
    if (published == Publish::cancelled) return {FileJobStatus::cancelled, {}, {}};
    if (published == Publish::stale_input) return verification_failure(verified);
    if (published == Publish::error) return {FileJobStatus::error, {}, "cache publication failed or commit outcome unknown", std::make_pair(id, key)};
    return {FileJobStatus::committed, result, {}};
  } catch (const detail::VirtualOpenCancelled&) { return {FileJobStatus::cancelled, {}, {}}; }
    catch (const Cancelled&) { return {FileJobStatus::cancelled, {}, {}}; }
    catch (const std::exception& e) { return {FileJobStatus::error, {}, e.what()}; }
}

StoredSnapshotJobResult analyze_snapshot_to_store(AnalysisStore& store, const std::filesystem::path& path,
  const std::string& engine, const std::string& config, std::stop_token stop,
  const std::function<void(std::uint64_t)>& progress) {
  try {
    if (engine.empty() || config.empty()) return {FileJobStatus::error, {}, "empty engine or configuration", {}};
    auto opened = SnapshotOpen::create(path, stop);
    if (opened.status == SnapshotStatus::cancelled) return {FileJobStatus::cancelled, {}, {}, {}};
    if (opened.status == SnapshotStatus::stale_input || opened.status == SnapshotStatus::unverifiable) return {FileJobStatus::stale_input, {}, opened.diagnostic, {}};
    if (opened.status != SnapshotStatus::ok) return {FileJobStatus::error, {}, opened.diagnostic, {}};
    auto modified = opened.value->modified_unix_ns();
    if (!modified) return {FileJobStatus::stale_input, {}, "original modification time cannot be represented", {}};
    FileVersion file{std::filesystem::absolute(path), opened.value->size(), *modified, opened.value->digest()};
    AnalysisIdentity identity{opened.value->digest(), engine, config, "offline"};
    auto found = store.find(file, identity, stop);
    if (found.outcome == FindOutcome::cancelled) return {FileJobStatus::cancelled, {}, {}, {}};
    if (found.outcome == FindOutcome::error) return {FileJobStatus::error, {}, found.diagnostic, {}};
    if (found.outcome == FindOutcome::hit) {
      if (!found.result) return {FileJobStatus::error, {}, "store returned hit without value", {}};
      auto verified = opened.value->verify(stop);
      if (verified == SnapshotStatus::cancelled || stop.stop_requested()) return {FileJobStatus::cancelled, {}, {}, {}};
      if (verified == SnapshotStatus::stale_input || verified == SnapshotStatus::unverifiable) return {FileJobStatus::stale_input, {}, {}, {}};
      if (verified != SnapshotStatus::ok) return {FileJobStatus::error, {}, "source revalidation failed", {}};
      return {FileJobStatus::cache_hit, std::move(found.result), {}, {}};
    }
    if (found.outcome != FindOutcome::miss) return {FileJobStatus::error, {}, "unknown store lookup outcome", {}};
    SnapshotSource source(*opened.value); std::uint64_t observed = 0;
    auto tempo = replay_snapshot(source, stop, [&](auto frames) { observed = frames; if (progress) progress(frames); });
    CachedAnalysis value; value.identity = identity; value.result.state = tempo.state; value.result.bpm = tempo.bpm;
    value.result.observed_through = {0, observed}; value.result.estimator_version = engine; value.result.configuration_id = config;
    class Verifier final : public FileVersionVerifier {
    public:
      Verifier(FileSnapshot& source, std::stop_token token) : source_(source), token_(token) {}
      FileVerificationResult verify() override {
        auto result = source_.verify(token_); cancelled = result == SnapshotStatus::cancelled;
        switch (result) {
          case SnapshotStatus::ok: return {FileVerificationOutcome::match, {}};
          case SnapshotStatus::stale_input: return {FileVerificationOutcome::changed, {}};
          case SnapshotStatus::unverifiable: return {FileVerificationOutcome::unverifiable, {}};
          case SnapshotStatus::cancelled: return {FileVerificationOutcome::error, "verification cancelled"};
          default: return {FileVerificationOutcome::error, "source verification failed"};
        }
      }
      bool cancelled = false;
    private:
      FileSnapshot& source_; std::stop_token token_;
    } verifier(*opened.value, stop);
    auto id = run_id(); auto published = store.publish(file, value, id, verifier, stop);
    if (published.outcome == PublishOutcome::committed) return {FileJobStatus::committed, value, {}, {}};
    if (published.outcome == PublishOutcome::cancelled || verifier.cancelled) return {FileJobStatus::cancelled, {}, {}, {}};
    if (published.outcome == PublishOutcome::stale_input) return {FileJobStatus::stale_input, {}, published.diagnostic, {}};
    return {FileJobStatus::error, {}, published.diagnostic, id};
  } catch (const detail::VirtualOpenCancelled&) { return {FileJobStatus::cancelled, {}, {}, {}}; }
    catch (const Cancelled&) { return {FileJobStatus::cancelled, {}, {}, {}}; }
    catch (const std::exception& e) { return {FileJobStatus::error, {}, e.what(), {}}; }
}

}
