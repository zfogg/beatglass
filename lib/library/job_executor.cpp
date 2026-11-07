// Private worker primitive. Integration into the public job API is pending.
#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <stop_token>
#include <thread>
#include <utility>

namespace beatglass::library_detail {
// A single worker owns execution. Capacity counts waiting jobs, not the running
// job. Higher priority runs first; equal priority remains FIFO. Destruction
// rejects new work, requests cancellation, discards queued work, and joins.
class JobExecutor {
public:
  using Work = std::function<void(std::stop_token)>;
  explicit JobExecutor(std::size_t capacity) : capacity_(capacity), worker_([this](std::stop_token stop) { run(stop); }) {}
  JobExecutor(const JobExecutor&) = delete;
  JobExecutor& operator=(const JobExecutor&) = delete;
  ~JobExecutor() { shutdown(); }

  bool submit(int priority, Work work) {
    if (!work) return false;
    std::lock_guard lock(mutex_);
    if (stopping_ || queue_.size() >= capacity_) return false;
    auto pos = std::find_if(queue_.begin(), queue_.end(), [priority](const Job& job) { return job.priority < priority; });
    queue_.insert(pos, Job{priority, std::move(work)});
    ready_.notify_one();
    return true;
  }

  // Control thread only, never from a job: waits for the active job to return.
  // Work must cooperate with its stop token; exceptions are contained.
  void shutdown() {
    {
      std::lock_guard lock(mutex_);
      if (stopping_) return;
      stopping_ = true;
      queue_.clear();
    }
    worker_.request_stop();
    ready_.notify_all();
    if (worker_.joinable()) worker_.join();
  }
private:
  struct Job { int priority; Work work; };
  void run(std::stop_token stop) {
    for (;;) {
      Work work;
      {
        std::unique_lock lock(mutex_);
        ready_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
        if (stopping_) return;
        work = std::move(queue_.front().work);
        queue_.pop_front();
      }
      try { work(stop); } catch (...) { /* Job adapter must report its own errors. */ }
    }
  }
  const std::size_t capacity_;
  std::mutex mutex_;
  std::condition_variable ready_;
  std::deque<Job> queue_;
  bool stopping_ = false;
  std::jthread worker_;
};
}
