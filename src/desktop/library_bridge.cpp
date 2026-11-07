#include "library_bridge.hpp"
#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>
using namespace beatglass::library_detail;
struct LibraryBridge::State {
  using Work = std::function<Result(TempoAnalysisStore&, std::stop_token)>;
  struct Request { Work work; Completion completion; std::uint64_t generation; std::stop_source stop; };
  std::mutex mutex;
  std::condition_variable ready;
  std::optional<Request> pending;
  std::stop_source active;
  std::uint64_t generation{};
  bool stopping{};
  Dispatch dispatch;
  std::jthread worker;
  void submit(Work work, Completion completion) {
    std::lock_guard lock(mutex);
    active.request_stop();
    if (pending) pending->stop.request_stop();
    pending = Request{std::move(work), std::move(completion), ++generation, {}};
    ready.notify_one();
  }
};
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
  { std::lock_guard lock(state_->mutex); state_->stopping = true; ++state_->generation; state_->active.request_stop(); if (state_->pending) state_->pending->stop.request_stop(); state_->pending.reset(); }
  state_->ready.notify_one();
  if (state_->worker.joinable()) state_->worker.join();
}
void LibraryBridge::cancel() {
  std::lock_guard lock(state_->mutex); ++state_->generation; state_->active.request_stop(); if (state_->pending) state_->pending->stop.request_stop(); state_->pending.reset();
}
void LibraryBridge::entries(CollectionQuery query, Completion completion) {
  state_->submit([query = std::move(query)](TempoAnalysisStore& store, std::stop_token stop) { Result r; r.value = store.entries(query, stop); return r; }, std::move(completion));
}
void LibraryBridge::detail(std::string path, Completion completion) {
  state_->submit([path = std::move(path)](TempoAnalysisStore& store, std::stop_token stop) { Result r; r.value = store.entry(path, stop); return r; }, std::move(completion));
}
void LibraryBridge::analyze(std::filesystem::path path, std::string engine, std::string configuration, std::uint64_t epoch, Completion completion) {
  state_->submit([path = std::move(path), engine = std::move(engine), configuration = std::move(configuration), epoch](TempoAnalysisStore& store, std::stop_token stop) {
    Result r; if (!epoch) { r.error = "analysis epoch must be nonzero"; return r; }
    r.value = analyze_snapshot_to_store(store, path, engine, configuration, epoch, stop); return r;
  }, std::move(completion));
}
