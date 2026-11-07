#include "library_bridge.hpp"
#include "beatglass/library/detail/file_snapshot.hpp"
#include <condition_variable>
#include <array>
#include <chrono>
#include <mutex>
#include <thread>
#include <utility>
using namespace beatglass::library_detail;
struct LibraryBridge::State {
  using Work = std::function<Result(TempoAnalysisStore&, std::stop_token)>;
  struct Request { Work work; Completion completion; std::uint64_t generation; std::size_t lane; std::stop_source stop; std::shared_ptr<Ticket> ticket; };
  std::mutex mutex;
  std::condition_variable ready;
  std::array<std::optional<Request>, 2> pending;
  std::array<std::stop_source, 2> active;
  std::array<std::uint64_t, 2> generation{};
  std::size_t next_lane{};
  bool stopping{};
  Dispatch dispatch;
  std::shared_ptr<beatglass::SourceEpochAllocator> epochs;
  std::jthread worker;
  void cancel_lane(std::size_t lane) {
    ++generation[lane]; active[lane].request_stop();
    if (pending[lane]) { pending[lane]->stop.request_stop(); Result cancelled; cancelled.cancelled = true; pending[lane]->ticket->finish(std::move(cancelled)); pending[lane].reset(); }
  }
  std::shared_ptr<Ticket> submit(std::size_t lane, Work work, Completion completion) {
    auto ticket = std::make_shared<Ticket>();
    std::lock_guard lock(mutex);
    cancel_lane(lane);
    pending[lane] = Request{std::move(work), std::move(completion), generation[lane], lane, {}, ticket};
    ready.notify_one();
    return ticket;
  }
};
std::optional<LibraryBridge::Result> LibraryBridge::Ticket::result() const { std::lock_guard lock(mutex_); return result_; }
void LibraryBridge::Ticket::finish(Result result) { std::lock_guard lock(mutex_); if (!result_) result_ = std::move(result); }
LibraryBridge::LibraryBridge(std::string database, Dispatch dispatch) : LibraryBridge(std::move(database), std::move(dispatch), {}) {}
LibraryBridge::LibraryBridge(std::string database, Dispatch dispatch, std::shared_ptr<beatglass::SourceEpochAllocator> epochs) : state_(std::make_shared<State>()) {
  state_->epochs = std::move(epochs);
  if (!dispatch) throw std::invalid_argument("dispatcher required");
  state_->dispatch = std::move(dispatch);
  auto state = state_;
  state_->worker = std::jthread([state, database = std::move(database)] {
    std::unique_ptr<TempoAnalysisStore> store;
    std::string open_error;
    try { store = std::make_unique<TempoAnalysisStore>(database); }
    catch (const std::exception& e) { open_error = e.what(); }
    catch (...) { open_error = "database initialization failed"; }
    for (;;) {
      std::optional<State::Request> request;
      {
        std::unique_lock lock(state->mutex);
        state->ready.wait(lock, [&] { return state->stopping || state->pending[0].has_value() || state->pending[1].has_value(); });
        if (state->stopping) return;
        auto lane = state->pending[state->next_lane] ? state->next_lane : 1 - state->next_lane;
        request = std::move(state->pending[lane]); state->pending[lane].reset(); state->active[lane] = request->stop; state->next_lane = 1 - lane;
      }
      Result result;
      try {
        if (!store) result.error = open_error;
        else result = request->work(*store, request->stop.get_token());
      } catch (const CollectionCancelled&) { result.cancelled = true; }
        catch (const std::exception& e) { result.error = e.what(); }
        catch (...) { result.error = "library operation failed"; }
      request->ticket->finish(result);
      try {
        std::weak_ptr<State> weak = state;
        state->dispatch([weak, generation = request->generation, lane = request->lane, completion = std::move(request->completion), result = std::move(result)]() mutable {
          auto state = weak.lock(); if (!state) return;
          { std::lock_guard lock(state->mutex); if (state->stopping || generation != state->generation[lane]) return; }
          if (completion) completion(std::move(result));
        });
      } catch (...) { /* A failed dispatcher cannot deliver a completion. */ }
    }
  });
}
LibraryBridge::~LibraryBridge() {
  { std::lock_guard lock(state_->mutex); state_->stopping = true; state_->cancel_lane(0); state_->cancel_lane(1); }
  state_->ready.notify_one();
  if (state_->worker.joinable()) state_->worker.join();
}
std::optional<std::uint64_t> LibraryBridge::new_session_epoch() { return state_->epochs ? state_->epochs->next() : std::nullopt; }
std::shared_ptr<LibraryBridge::Ticket> LibraryBridge::start_analysis(std::filesystem::path path, std::string engine, std::string configuration, Completion completion) {
  auto epoch = new_session_epoch(); return analyze(std::move(path), std::move(engine), std::move(configuration), epoch.value_or(0), std::move(completion));
}
std::shared_ptr<LibraryBridge::Ticket> LibraryBridge::start_indexed_analysis(std::string key, std::filesystem::path path, std::string engine, std::string configuration, Completion completion) {
  auto epoch = new_session_epoch(); return analyze_indexed(std::move(key), std::move(path), std::move(engine), std::move(configuration), epoch.value_or(0), std::move(completion));
}
void LibraryBridge::cancel_browse() { std::lock_guard lock(state_->mutex); state_->cancel_lane(0); }
void LibraryBridge::cancel_analysis() { std::lock_guard lock(state_->mutex); state_->cancel_lane(1); }
void LibraryBridge::cancel() { std::lock_guard lock(state_->mutex); state_->cancel_lane(0); state_->cancel_lane(1); }
std::shared_ptr<LibraryBridge::Ticket> LibraryBridge::entries(CollectionQuery query, Completion completion) {
  return state_->submit(0, [query = std::move(query)](TempoAnalysisStore& store, std::stop_token stop) { Result r; r.value = store.entries(query, stop); return r; }, std::move(completion));
}
std::shared_ptr<LibraryBridge::Ticket> LibraryBridge::detail(std::string path, Completion completion) {
  return state_->submit(0, [path = std::move(path)](TempoAnalysisStore& store, std::stop_token stop) { Result r; r.value = store.entry(path, stop); return r; }, std::move(completion));
}
std::shared_ptr<LibraryBridge::Ticket> LibraryBridge::analyze(std::filesystem::path path, std::string engine, std::string configuration, std::uint64_t epoch, Completion completion) {
  return state_->submit(1, [path = std::move(path), engine = std::move(engine), configuration = std::move(configuration), epoch](TempoAnalysisStore& store, std::stop_token stop) {
    Result r; if (!epoch) { r.error = "analysis epoch must be nonzero"; return r; }
    r.value = analyze_snapshot_to_store(store, path, engine, configuration, epoch, stop); return r;
  }, std::move(completion));
}

namespace {
bool key_matches(const std::string& key, const std::filesystem::path& path) {
  return std::filesystem::absolute(std::filesystem::u8path(key)).lexically_normal() == std::filesystem::absolute(path).lexically_normal();
}
LibraryBridge::Result lookup_active(TempoAnalysisStore& store, const std::filesystem::path& path, const beatglass::AnalysisIdentity& identity, std::uint64_t epoch, std::stop_token stop) {
    LibraryBridge::Result result; LibraryBridge::Lookup lookup;
    if (!epoch || identity.content_hash.empty() || identity.engine_version.empty() || identity.configuration_id.empty() || identity.mode != "offline") {
      lookup.diagnostic = "invalid lookup identity or epoch"; result.value = std::move(lookup); return result;
    }
    auto opened = SnapshotOpen::create(path, stop);
    if (opened.status == SnapshotStatus::cancelled) lookup.status = LibraryBridge::LookupStatus::cancelled;
    else if (opened.status == SnapshotStatus::stale_input || opened.status == SnapshotStatus::unverifiable) lookup.status = LibraryBridge::LookupStatus::changed;
    else if (opened.status != SnapshotStatus::ok) lookup.diagnostic = opened.diagnostic;
    else if (opened.value->digest() != identity.content_hash) lookup.status = LibraryBridge::LookupStatus::changed;
    else {
      auto modified = opened.value->modified_unix_ns();
      if (!modified) { lookup.status = LibraryBridge::LookupStatus::changed; lookup.diagnostic = "source time cannot be represented"; }
      else {
        beatglass::FileVersion file{std::filesystem::absolute(path), opened.value->size(), *modified, opened.value->digest()};
        auto found = store.find(file, identity, stop);
        if (found.outcome == beatglass::FindOutcome::cancelled) lookup.status = LibraryBridge::LookupStatus::cancelled;
        else if (found.outcome == beatglass::FindOutcome::error) lookup.diagnostic = found.diagnostic;
        else {
          auto verified = opened.value->verify(stop);
          if (verified == SnapshotStatus::cancelled || stop.stop_requested()) lookup.status = LibraryBridge::LookupStatus::cancelled;
          else if (verified == SnapshotStatus::stale_input || verified == SnapshotStatus::unverifiable) lookup.status = LibraryBridge::LookupStatus::changed;
          else if (verified != SnapshotStatus::ok) lookup.diagnostic = "source verification failed";
          else if (found.outcome == beatglass::FindOutcome::miss) lookup.status = LibraryBridge::LookupStatus::miss;
          else if (!found.result) lookup.diagnostic = "store returned hit without value";
          else {
            auto& snapshot = found.result->result; auto previous = snapshot.observed_through.epoch;
            bool valid = true;
            for (auto& beat : snapshot.beats) { if (beat.time.epoch != previous) valid = false; beat.time.epoch = epoch; }
            for (auto& chord : snapshot.chords) { if (chord.start.epoch != previous || chord.end.epoch != previous) valid = false; chord.start.epoch = epoch; chord.end.epoch = epoch; }
            if (!valid) lookup.diagnostic = "cached position epoch mismatch";
            else { snapshot.observed_through.epoch = epoch; snapshot.publication_monotonic_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); lookup.status = LibraryBridge::LookupStatus::hit; lookup.value = std::move(found.result); }
          }
        }
      }
    }
    result.value = std::move(lookup); return result;
}
}
std::shared_ptr<LibraryBridge::Ticket> LibraryBridge::lookup(std::filesystem::path path, beatglass::AnalysisIdentity identity, std::uint64_t epoch, Completion completion) {
  return state_->submit(0, [path = std::move(path), identity = std::move(identity), epoch](TempoAnalysisStore& store, std::stop_token stop) {
    return lookup_active(store, path, identity, epoch, stop);
  }, std::move(completion));
}
std::shared_ptr<LibraryBridge::Ticket> LibraryBridge::analyze_indexed(std::string key, std::filesystem::path path, std::string engine, std::string configuration, std::uint64_t epoch, Completion completion) {
  return state_->submit(1, [key = std::move(key), path = std::move(path), engine = std::move(engine), configuration = std::move(configuration), epoch](TempoAnalysisStore& store, std::stop_token stop) {
    Result result;
    if (!epoch || !key_matches(key, path)) { result.error = "invalid epoch or index path mismatch"; return result; }
    auto entry = store.entry(key, stop);
    if (!entry) { result.error = "indexed entry not found"; return result; }
    auto analysis = analyze_indexed_snapshot_to_store(store, key, path, engine, configuration, epoch, stop);
    result.value = std::move(analysis); return result;
  }, std::move(completion));
}
std::shared_ptr<LibraryBridge::Ticket> LibraryBridge::lookup_indexed(std::string key, std::filesystem::path path, std::string engine, std::string configuration, std::uint64_t epoch, Completion completion) {
  return state_->submit(0, [key = std::move(key), path = std::move(path), engine = std::move(engine), configuration = std::move(configuration), epoch](TempoAnalysisStore& store, std::stop_token stop) {
    Result result;
    if (!epoch || engine.empty() || configuration.empty() || !key_matches(key, path)) { result.error = "invalid identity, epoch or index path mismatch"; return result; }
    auto entry = store.entry(key, stop);
    if (!entry || entry->content_hash.empty()) { Lookup absent; absent.status = LookupStatus::miss; result.value = std::move(absent); return result; }
    return lookup_active(store, path, snapshot_tempo_identity(entry->content_hash, engine, configuration), epoch, stop);
  }, std::move(completion));
}

std::shared_ptr<LibraryBridge::Ticket> LibraryBridge::offline_page(std::string key, std::filesystem::path path, OfflineKey identity, std::string run, std::size_t beat_offset, std::size_t beat_count, std::size_t window_offset, std::size_t window_count, std::uint64_t epoch, Completion completion) {
  return state_->submit(0, [key = std::move(key), path = std::move(path), identity = std::move(identity), run = std::move(run), beat_offset, beat_count, window_offset, window_count, epoch](TempoAnalysisStore& store, std::stop_token stop) {
    Result result; OfflineReadResult read;
    if (!epoch || !key_matches(key, path)) { read.diagnostic = "invalid epoch or index path mismatch"; result.value = std::move(read); return result; }
    auto entry = store.entry(key, stop);
    if (!entry || entry->content_hash.empty()) { read.status = OfflineReadStatus::miss; result.value = std::move(read); return result; }
    if (entry->content_hash != identity.content_hash) { read.status = OfflineReadStatus::changed; result.value = std::move(read); return result; }
    auto opened = SnapshotOpen::create(path, stop);
    if (opened.status == SnapshotStatus::cancelled) read.status = OfflineReadStatus::cancelled;
    else if (opened.status == SnapshotStatus::stale_input || opened.status == SnapshotStatus::unverifiable) read.status = OfflineReadStatus::changed;
    else if (opened.status != SnapshotStatus::ok) read.diagnostic = opened.diagnostic;
    else if (opened.value->digest() != identity.content_hash) read.status = OfflineReadStatus::changed;
    else {
      auto modified = opened.value->modified_unix_ns();
      if (!modified) read.status = OfflineReadStatus::changed;
      else {
        beatglass::FileVersion version{std::filesystem::absolute(path), opened.value->size(), *modified, opened.value->digest()};
        class Verifier final : public beatglass::FileVersionVerifier {
          FileSnapshot& snapshot_;
        public:
          explicit Verifier(FileSnapshot& snapshot) : snapshot_(snapshot) {}
          beatglass::FileVerificationResult verify(std::stop_token stop) override {
            switch (snapshot_.verify(stop)) {
              case SnapshotStatus::ok: return {beatglass::FileVerificationOutcome::match, {}};
              case SnapshotStatus::cancelled: return {beatglass::FileVerificationOutcome::cancelled, {}};
              case SnapshotStatus::stale_input: return {beatglass::FileVerificationOutcome::changed, {}};
              case SnapshotStatus::unverifiable: return {beatglass::FileVerificationOutcome::unverifiable, {}};
              default: return {beatglass::FileVerificationOutcome::error, "snapshot verification failed"};
            }
          }
        } verifier(*opened.value);
        OfflineArchive archive(store.offline_cache());
        read = archive.page(version, identity, run, beat_offset, beat_count, window_offset, window_count, epoch, verifier, stop);
      }
    }
    result.value = std::move(read); return result;
  }, std::move(completion));
}
#ifdef BEATGLASS_PRIVATE_COMPOSITE_BRIDGE
std::shared_ptr<LibraryBridge::Ticket> LibraryBridge::start_composite(std::filesystem::path path, Completion completion) {
  auto epoch = new_session_epoch(); return analyze_composite(std::move(path), epoch.value_or(0), std::move(completion));
}
std::shared_ptr<LibraryBridge::Ticket> LibraryBridge::analyze_composite(std::filesystem::path path, std::uint64_t epoch, Completion completion) {
  return state_->submit(1, [path = std::move(path), epoch](TempoAnalysisStore&, std::stop_token stop) {
    Result result;
    result.value = analyze_snapshot_music(path, epoch, {100000, 10000, 256}, stop);
    return result;
  }, std::move(completion));
}
#endif
