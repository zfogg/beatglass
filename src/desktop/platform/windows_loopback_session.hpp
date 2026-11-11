#pragma once
#include "windows_loopback.hpp"
#include "beatglass/capture/source_lifecycle.hpp"
#include <functional>
#include <memory>
#include <string>

namespace beatglass::windows {
/** Provisional single-control-thread adapter from WASAPI transitions to the reviewed source lifecycle. The owner must retain the process epoch allocator and invalidate callback across source recreation. */
class LoopbackSession {
public:
  /** Borrow the process epoch allocator and retain an invalidation callback. The callback runs synchronously on the control thread, must not throw, and must clear publication and reject in-flight work. An empty callback prevents start. */
  LoopbackSession(SourceEpochAllocator& epochs, std::function<void()> invalidate);
  /** Test seam: take exclusive ownership of an injected backend. A null backend prevents start. All methods are called on the sole control thread. */
  LoopbackSession(SourceEpochAllocator& epochs, std::function<void()> invalidate, std::unique_ptr<Loopback> backend);
  // Documented manually in the API reference due to Hawkmoth destructor parsing.
  ~LoopbackSession();
  /** Start a selected endpoint with a fresh epoch. True means accepted, not running. Call on the control thread while stopped; stop after an error before retrying. */
  bool start(const std::string& endpoint);
  /** Claim a caller-owned attempt before allocating an epoch. Cancellation may be requested concurrently and is passed to capture; stop and all other session calls remain on the control worker. A native call already in progress is not preempted. A cancellation racing a poll or pop may still return a packet; the consumer must gate delivery against synchronized authorization or generation state. */
  bool start(const std::string& endpoint, std::shared_ptr<LoopbackStartAuthorization> authorization);
  /** Apply pending transitions and synchronously invalidate on interruption, error, ordering failure or queue loss. Queue loss fails the attempt and stops capture, discarding the complete backlog; call stop before a fresh-epoch restart. Call on the control thread. */
  void poll();
  /** Apply pending transitions and copy a packet only for the currently running epoch. A discontinuous or untimed packet first triggers invalidation. Any previously returned packet remains subject to the owner's invalidation token. Call on the control thread. */
  bool pop(LoopbackPacket& packet);
  /** Invalidate evidence, join capture, discard queued messages and stop the lifecycle. Call on the control thread. */
  void stop();
  /** Share the backend loss token; call on the control worker. Retained references outlive the session. */
  std::shared_ptr<const LoopbackLossToken> lossToken() const noexcept;
  /** Read lifecycle state on the control thread. */
  SourceState state() const;
  /** Return the native HRESULT for the most recent failed or interrupted stream, or zero when unavailable. */
  std::int32_t error_code() const;
private:
  void invalidate();
  SourceEpochAllocator& epochs_;
  std::function<void()> invalidate_;
  SourceLifecycle lifecycle_;
  std::unique_ptr<Loopback> backend_;
  std::shared_ptr<LoopbackStartAuthorization> authorization_;
  SourceAttempt attempt_{};
  std::uint64_t epoch_{};
  std::uint64_t dropped_{};
};
}
