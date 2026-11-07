#pragma once
#include <cstddef>
#include <functional>
#include <memory>
#include <stop_token>
namespace beatglass::library_detail {
/** Provisional single-worker queue; no stable public API. Calls to shutdown and destruction must be serialized on a control thread, never from a job. Jobs must cooperate with stop requests. */
class JobExecutor {
public:
  /** A job invoked on the worker with a cooperative shutdown token. */
  using Work = std::function<void(std::stop_token)>;
  /** Start one worker; capacity bounds queued jobs, excluding the running job. */
  explicit JobExecutor(std::size_t capacity);
  /** Request shutdown, discard queued jobs, and join the worker. */
  ~JobExecutor();
  JobExecutor(const JobExecutor&) = delete;
  JobExecutor& operator=(const JobExecutor&) = delete;
  /** Queue a nonempty job if space is available. Higher priority runs first, with FIFO for equal priority. Thread-safe until shutdown begins. */
  bool submit(int priority, Work work);
  /** Reject and discard queued jobs, request active cancellation, and join. May block until active work returns. */
  void shutdown();
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
