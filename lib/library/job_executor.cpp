#include "beatglass/library/detail/job_executor.hpp"
#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <utility>
namespace beatglass::library_detail {
struct JobHandle::State {
  mutable std::mutex mutex;
  std::stop_source stop;
  TrackedState status{TrackedState::queued};
  std::uint64_t frames{};
  std::function<void()> on_queued_cancel;
};
JobHandle::JobHandle(std::shared_ptr<State> state) : state_(std::move(state)) {}
void JobHandle::cancel() {
  std::function<void()> queued_callback;
  {
    std::lock_guard lock(state_->mutex);
    if (state_->status != TrackedState::queued && state_->status != TrackedState::running) return;
    if (state_->status == TrackedState::queued) { state_->status = TrackedState::cancelled; queued_callback = std::move(state_->on_queued_cancel); }
  }
  // A cancellation begun while running can race terminal publication. Never
  // invoke stop callbacks while holding the state mutex.
  state_->stop.request_stop();
  if (queued_callback) { try { queued_callback(); } catch (...) {} }
}
TrackedState JobHandle::state() const { std::lock_guard lock(state_->mutex); return state_->status; }
std::uint64_t JobHandle::frames() const { std::lock_guard lock(state_->mutex); return state_->frames; }
struct JobExecutor::Impl {
  struct Job { int priority; Work work; std::shared_ptr<JobHandle> handle; };
  explicit Impl(std::size_t n) : capacity(n), worker([this](std::stop_token stop) { run(stop); }) {}
  void run(std::stop_token stop) {
    for (;;) {
      Work work;
      {
        std::unique_lock lock(mutex);
        ready.wait(lock, [this] { return stopping || !queue.empty(); });
        if (stopping) return;
        work = std::move(queue.front().work); queue.pop_front();
      }
      try { work(stop); } catch (...) { /* Adapter must report its own errors. */ }
    }
  }
  std::size_t capacity;
  std::mutex mutex;
  std::condition_variable ready;
  std::deque<Job> queue;
  bool stopping = false;
  void prune_cancelled() {
    queue.erase(std::remove_if(queue.begin(), queue.end(), [](const Job& job) {
      return job.handle && job.handle->state() == TrackedState::cancelled;
    }), queue.end());
  }
  std::jthread worker;
};
JobExecutor::JobExecutor(std::size_t capacity) : impl_(std::make_unique<Impl>(capacity)) {}
JobExecutor::~JobExecutor() { shutdown(); }
bool JobExecutor::submit(int priority, Work work) {
  if (!work) return false;
  std::lock_guard lock(impl_->mutex);
  impl_->prune_cancelled();
  if (impl_->stopping || impl_->queue.size() >= impl_->capacity) return false;
  auto pos = std::find_if(impl_->queue.begin(), impl_->queue.end(), [priority](const Impl::Job& job) { return job.priority < priority; });
  impl_->queue.insert(pos, Impl::Job{priority, std::move(work), {}});
  impl_->ready.notify_one();
  return true;
}
std::shared_ptr<JobHandle> JobExecutor::submit_tracked(int priority, TrackedWork work, std::function<void()> on_queued_cancel) {
  if (!work) return {};
  auto state = std::make_shared<JobHandle::State>();
  state->on_queued_cancel = std::move(on_queued_cancel);
  auto handle = std::shared_ptr<JobHandle>(new JobHandle(state));
  Work wrapped = [state, work = std::move(work)](std::stop_token shutdown) {
    {
      std::lock_guard lock(state->mutex);
      if (state->status == TrackedState::cancelled) return;
      state->status = TrackedState::running;
    }
    std::stop_callback on_shutdown(shutdown, [state] { state->stop.request_stop(); });
    auto progress = [state](std::uint64_t frames) { std::lock_guard lock(state->mutex); if (state->status == TrackedState::running && frames > state->frames) state->frames = frames; };
    TrackedState result = TrackedState::failed;
    try { result = work(state->stop.get_token(), progress); } catch (...) {}
    if (result != TrackedState::completed && result != TrackedState::cancelled && result != TrackedState::failed) result = TrackedState::failed;
    std::lock_guard lock(state->mutex); state->status = result;
  };
  std::lock_guard lock(impl_->mutex);
  impl_->prune_cancelled();
  if (impl_->stopping || impl_->queue.size() >= impl_->capacity) return {};
  auto pos = std::find_if(impl_->queue.begin(), impl_->queue.end(), [priority](const Impl::Job& job) { return job.priority < priority; });
  impl_->queue.insert(pos, Impl::Job{priority, std::move(wrapped), handle});
  impl_->ready.notify_one();
  return handle;
}
void JobExecutor::shutdown() {
  std::deque<Impl::Job> discarded;
  {
    std::lock_guard lock(impl_->mutex);
    if (impl_->stopping) return;
    impl_->stopping = true;
    discarded.swap(impl_->queue);
  }
  impl_->worker.request_stop();
  impl_->ready.notify_all();
  for (const auto& job : discarded) if (job.handle) job.handle->cancel();
  discarded.clear();
  if (impl_->worker.joinable()) impl_->worker.join();
}
}
