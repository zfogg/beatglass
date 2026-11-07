#pragma once
#include "windows_endpoints.hpp"
#include "windows_live_tempo.hpp"
#include "beatglass/core/presentation_gate.hpp"
#include <chrono>
#include <functional>
#include <optional>

namespace beatglass::windows {
/** One owned default follower delivery. Token and stamp are captured with the update; clears have no stamp. */
struct StampedTempoUpdate : PresentationUpdate {
  /** Token retained for the source that produced the update. */
  std::shared_ptr<const LoopbackLossToken> loss_token;
  /** Original evidence stamp, absent on a clear. */
  std::optional<std::uint64_t> loss_stamp;
};

/** Worker-owned default-render-endpoint follower and provisional revisioned tempo producer. All methods run on one worker/control thread; the GUI must transfer and apply updates on its own thread. Call stop and deliver its clear before destroying the follower. */
class DefaultTempoFollower {
public:
  /** Control status; running does not imply that a BPM has been acquired. */
  enum class Status { waiting, starting, running, error };
  /** Borrow a process epoch allocator. An empty discovery function uses Windows discovery. An optional start guard synchronously decides whether to invoke a capture start on this worker; it must not retain the callable. */
  explicit DefaultTempoFollower(SourceEpochAllocator& epochs, std::function<EndpointResult()> discovery = {}, std::function<bool(const std::function<bool()>&)> startGuard = {});
  /** Enable default following; the next tick performs discovery. Return false after fatal revision exhaustion. */
  bool start();
  /** Poll capture and periodically rediscover the multimedia default. now must be monotonic. On a new or lost endpoint, clear old evidence before stopping and restarting with a fresh epoch. False means a fatal failure or revision exhaustion; the owner must stop and clear the GUI using an external session boundary. */
  bool tick(std::chrono::steady_clock::time_point now);
  /** Return and remove the newest coalesced update, including clears. Transfer it to the GUI using an ordered/coalescing mechanism that retains clears. */
  std::optional<StampedTempoUpdate> takeUpdate();
  /** Disable following, invalidate the current value and stop capture. A later start is required before tick can capture again. */
  void stop();
  /** Current selected endpoint ID; empty while waiting or recovering. */
  const std::string& endpoint() const;
  /** Current worker-owned status; transfer it to the GUI through the owner's delivery mechanism. */
  Status status() const;
  /** Most recent status diagnostic, empty outside an error. */
  const std::string& diagnostic() const;
private:
  friend struct DefaultTempoFollowerTestAccess;
  bool publishClear();
  bool publishTempo(float bpm, EventTime observed);
  bool nextRevision();
  LiveTempo tempo_;
  std::function<EndpointResult()> discovery_;
  std::function<bool(const std::function<bool()>&)> start_guard_;
  std::string endpoint_;
  std::string diagnostic_;
  std::chrono::steady_clock::time_point next_probe_{};
  std::optional<StampedTempoUpdate> pending_;
  std::optional<float> published_bpm_;
  std::uint64_t revision_{};
  std::uint64_t last_invalidation_{};
  bool exhausted_{};
  bool enabled_{};
  Status status_{Status::waiting};
};
}
