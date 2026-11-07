#include "library_bridge.hpp"
#include "beatglass/library/detail/file_snapshot.hpp"
#include <condition_variable>
#include <chrono>
#include <mutex>
#include <thread>
#include <utility>
using namespace beatglass::library_detail;
struct LibraryBridge::State {
  using Work = std::function<Result(TempoAnalysisStore&, std::stop_token)>;
  struct Request { Work work; Completion completion; std::uint64_t generation; std::stop_source stop; std::shared_ptr<Ticket> ticket; };
  std::mutex mutex;
  std::condition_variable ready;
  std::optional<Request> pending;
  std::stop_source active;
  std::uint64_t generation{};
  bool stopping{};
  Dispatch dispatch;
  std::jthread worker;
  std::shared_ptr<Ticket> submit(Work work, Completion completion) {
    auto ticket = std::make_shared<Ticket>();
    std::lock_guard lock(mutex);
    active.request_stop();
    if (pending) { pending->stop.request_stop(); Result cancelled; cancelled.cancelled = true; pending->ticket->finish(std::move(cancelled)); }
    pending = Request{std::move(work), std::move(completion), ++generation, {}, ticket};
    ready.notify_one();
    return ticket;
  }
};
std::optional<LibraryBridge::Result> LibraryBridge::Ticket::result() const { std::lock_guard lock(mutex_); return result_; }
void LibraryBridge::Ticket::finish(Result result) { std::lock_guard lock(mutex_); if (!result_) result_ = std::move(result); }
LibraryBridge::LibraryBridge(std::string database, Dispatch dispatch) : state_(std::make_shared<State>()) {
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
        state->ready.wait(lock, [&] { return state->stopping || state->pending.has_value(); });
        if (state->stopping) return;
        request = std::move(state->pending); state->pending.reset(); state->active = request->stop;
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
        state->dispatch([weak, generation = request->generation, completion = std::move(request->completion), result = std::move(result)]() mutable {
          auto state = weak.lock(); if (!state) return;
          { std::lock_guard lock(state->mutex); if (state->stopping || generation != state->generation) return; }
          if (completion) completion(std::move(result));
        });
      } catch (...) { /* A failed dispatcher cannot deliver a completion. */ }
    }
  });
}
LibraryBridge::~LibraryBridge() {
  { std::lock_guard lock(state_->mutex); state_->stopping = true; ++state_->generation; state_->active.request_stop(); if (state_->pending) { state_->pending->stop.request_stop(); Result cancelled; cancelled.cancelled = true; state_->pending->ticket->finish(std::move(cancelled)); } state_->pending.reset(); }
  state_->ready.notify_one();
  if (state_->worker.joinable()) state_->worker.join();
}
void LibraryBridge::cancel() {
  std::lock_guard lock(state_->mutex); ++state_->generation; state_->active.request_stop(); if (state_->pending) { state_->pending->stop.request_stop(); Result cancelled; cancelled.cancelled = true; state_->pending->ticket->finish(std::move(cancelled)); } state_->pending.reset();
}
std::shared_ptr<LibraryBridge::Ticket> LibraryBridge::entries(CollectionQuery query, Completion completion) {
  return state_->submit([query = std::move(query)](TempoAnalysisStore& store, std::stop_token stop) { Result r; r.value = store.entries(query, stop); return r; }, std::move(completion));
}
std::shared_ptr<LibraryBridge::Ticket> LibraryBridge::detail(std::string path, Completion completion) {
  return state_->submit([path = std::move(path)](TempoAnalysisStore& store, std::stop_token stop) { Result r; r.value = store.entry(path, stop); return r; }, std::move(completion));
}
std::shared_ptr<LibraryBridge::Ticket> LibraryBridge::analyze(std::filesystem::path path, std::string engine, std::string configuration, std::uint64_t epoch, Completion completion) {
  return state_->submit([path = std::move(path), engine = std::move(engine), configuration = std::move(configuration), epoch](TempoAnalysisStore& store, std::stop_token stop) {
    Result r; if (!epoch) { r.error = "analysis epoch must be nonzero"; return r; }
    r.value = analyze_snapshot_to_store(store, path, engine, configuration, epoch, stop); return r;
  }, std::move(completion));
}

std::shared_ptr<LibraryBridge::Ticket> LibraryBridge::lookup(std::filesystem::path path, beatglass::AnalysisIdentity identity, std::uint64_t epoch, Completion completion) {
  return state_->submit([path = std::move(path), identity = std::move(identity), epoch](TempoAnalysisStore& store, std::stop_token stop) {
    Result result; Lookup lookup;
    if (!epoch || identity.content_hash.empty() || identity.engine_version.empty() || identity.configuration_id.empty() || identity.mode != "offline") {
      lookup.diagnostic = "invalid lookup identity or epoch"; result.value = std::move(lookup); return result;
    }
    auto opened = SnapshotOpen::create(path, stop);
    if (opened.status == SnapshotStatus::cancelled) lookup.status = LookupStatus::cancelled;
    else if (opened.status == SnapshotStatus::stale_input || opened.status == SnapshotStatus::unverifiable) lookup.status = LookupStatus::changed;
    else if (opened.status != SnapshotStatus::ok) lookup.diagnostic = opened.diagnostic;
    else if (opened.value->digest() != identity.content_hash) lookup.status = LookupStatus::changed;
    else {
      auto modified = opened.value->modified_unix_ns();
      if (!modified) { lookup.status = LookupStatus::changed; lookup.diagnostic = "source time cannot be represented"; }
      else {
        beatglass::FileVersion file{std::filesystem::absolute(path), opened.value->size(), *modified, opened.value->digest()};
        auto found = store.find(file, identity, stop);
        if (found.outcome == beatglass::FindOutcome::cancelled) lookup.status = LookupStatus::cancelled;
        else if (found.outcome == beatglass::FindOutcome::error) lookup.diagnostic = found.diagnostic;
        else {
          auto verified = opened.value->verify(stop);
          if (verified == SnapshotStatus::cancelled || stop.stop_requested()) lookup.status = LookupStatus::cancelled;
          else if (verified == SnapshotStatus::stale_input || verified == SnapshotStatus::unverifiable) lookup.status = LookupStatus::changed;
          else if (verified != SnapshotStatus::ok) lookup.diagnostic = "source verification failed";
          else if (found.outcome == beatglass::FindOutcome::miss) lookup.status = LookupStatus::miss;
          else if (!found.result) lookup.diagnostic = "store returned hit without value";
          else {
            auto& snapshot = found.result->result; auto previous = snapshot.observed_through.epoch;
            bool valid = true;
            for (auto& beat : snapshot.beats) { if (beat.time.epoch != previous) valid = false; beat.time.epoch = epoch; }
            for (auto& chord : snapshot.chords) { if (chord.start.epoch != previous || chord.end.epoch != previous) valid = false; chord.start.epoch = epoch; chord.end.epoch = epoch; }
            if (!valid) lookup.diagnostic = "cached position epoch mismatch";
            else { snapshot.observed_through.epoch = epoch; snapshot.publication_monotonic_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); lookup.status = LookupStatus::hit; lookup.value = std::move(found.result); }
          }
        }
      }
    }
    result.value = std::move(lookup); return result;
  }, std::move(completion));
}
