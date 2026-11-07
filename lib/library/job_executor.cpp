#include "beatglass/library/detail/job_executor.hpp"
#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <utility>
namespace beatglass::library_detail {
struct JobExecutor::Impl {
  struct Job { int priority; Work work; };
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
  std::jthread worker;
};
JobExecutor::JobExecutor(std::size_t capacity) : impl_(std::make_unique<Impl>(capacity)) {}
JobExecutor::~JobExecutor() { shutdown(); }
bool JobExecutor::submit(int priority, Work work) {
  if (!work) return false;
  std::lock_guard lock(impl_->mutex);
  if (impl_->stopping || impl_->queue.size() >= impl_->capacity) return false;
  auto pos = std::find_if(impl_->queue.begin(), impl_->queue.end(), [priority](const Impl::Job& job) { return job.priority < priority; });
  impl_->queue.insert(pos, Impl::Job{priority, std::move(work)});
  impl_->ready.notify_one();
  return true;
}
void JobExecutor::shutdown() {
  {
    std::lock_guard lock(impl_->mutex);
    if (impl_->stopping) return;
    impl_->stopping = true;
    impl_->queue.clear();
  }
  impl_->worker.request_stop();
  impl_->ready.notify_all();
  if (impl_->worker.joinable()) impl_->worker.join();
}
}
