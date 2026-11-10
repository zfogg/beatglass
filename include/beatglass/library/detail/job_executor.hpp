#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <stop_token>
namespace beatglass::library_detail {
/** Observable state of a tracked job. */
enum class TrackedState {
  /** Waiting for worker. */ queued,
  /** Executing on worker. */ running,
  /** Work reports successful completion. */ completed,
  /** Work was cancelled. */ cancelled,
  /** Work failed or threw. */ failed
};
/** Thread-safe retained view of one tracked job; may outlive the executor. */
class JobHandle {
public:
  /** Request cooperative cancellation; a queued job is marked cancelled immediately. A call observing a terminal state does nothing. A request begun while work is running can race terminal publication; stop callbacks run without the handle mutex and may finish after terminal state becomes observable. */
  void cancel();
  /** Copy current state. */ TrackedState state() const;
  /** Copy the latest monotonic frame count. */ std::uint64_t frames() const;
private:
  friend class JobExecutor;
  struct State;
  explicit JobHandle(std::shared_ptr<State> state);
  std::shared_ptr<State> state_;
};
/** Provisional single-worker queue; no stable public API. Calls to shutdown and destruction must be serialized on a control thread, never from a job. Jobs must cooperate with stop requests. */
class JobExecutor {
public:
  /** A job invoked on the worker with a cooperative shutdown token. */
  using Work = std::function<void(std::stop_token)>;
  /** Tracked work reports its terminal outcome itself, so late cancellation cannot relabel a successful commit. The progress callback must not be retained after work returns; calls are synchronous and monotonic, and late calls cannot update terminal state. */
  using TrackedWork = std::function<TrackedState(std::stop_token, const std::function<void(std::uint64_t)>&)>;
  /**
   * Start one worker; capacity bounds queued jobs, excluding the running job.
   * A zero capacity executor is valid but rejects every submission until it is
   * shut down.
   */
  explicit JobExecutor(std::size_t capacity);
  /**
   * Request shutdown, discard queued jobs, and join the worker. Destruction
   * performs the same operation; repeated shutdown calls are harmless.
   */
  ~JobExecutor();
  JobExecutor(const JobExecutor&) = delete;
  JobExecutor& operator=(const JobExecutor&) = delete;
  /**
   * Queue a nonempty job if space is available. Higher priority runs first,
   * with FIFO for equal priority. Thread-safe until shutdown begins. A thrown
   * exception from `work` is contained by the worker and does not escape the
   * executor; callers that need an outcome should use tracked work.
   */
  bool submit(int priority, Work work);
  /** Queue tracked work or return empty if full, stopping, or empty. A cancelled queued job occupies capacity until removed by the worker or shutdown. */
  std::shared_ptr<JobHandle> submit_tracked(int priority, TrackedWork work, std::function<void()> on_queued_cancel = {});
  /** Reject new jobs, request active cancellation, then cancel and destroy queued jobs outside the queue lock and join. May block until active work or queued closure destruction returns. Stop callbacks execute without the queue lock. */
  void shutdown();
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
