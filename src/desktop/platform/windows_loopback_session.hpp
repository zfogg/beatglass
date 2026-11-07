#pragma once
#include "windows_loopback.hpp"
#include "beatglass/capture/source_lifecycle.hpp"
#include <functional>
#include <string>

namespace beatglass::windows {
/** Provisional single-control-thread adapter from WASAPI transitions to the reviewed source lifecycle. The owner must retain the process epoch allocator and invalidate callback across source recreation. */
class LoopbackSession {
public:
  /** Borrow the process epoch allocator and retain an invalidation callback. The callback runs synchronously on the control thread, must not throw, and must clear publication and reject in-flight work. An empty callback prevents start. */
  LoopbackSession(SourceEpochAllocator& epochs, std::function<void()> invalidate);
  // Documented manually in the API reference due to Hawkmoth destructor parsing.
  ~LoopbackSession();
  /** Start a selected endpoint with a fresh epoch. True means accepted, not running. Call on the control thread while stopped; stop after an error before retrying. */
  bool start(const std::string& endpoint);
  /** Apply pending ordered transitions and synchronously invalidate on interruption, error, ordering failure or newly observed queue loss. Call frequently on the control thread. */
  void poll();
  /** Apply pending transitions and copy a packet only for the currently running epoch. A discontinuous or untimed packet first triggers invalidation. Any previously returned packet remains subject to the owner's invalidation token. Call on the control thread. */
  bool pop(LoopbackPacket& packet);
  /** Invalidate evidence, join capture, discard queued messages and stop the lifecycle. Call on the control thread. */
  void stop();
  /** Read lifecycle state on the control thread. */
  SourceState state() const;
private:
  void invalidate();
  SourceEpochAllocator& epochs_;
  std::function<void()> invalidate_;
  SourceLifecycle lifecycle_;
  Loopback backend_;
  SourceAttempt attempt_{};
  std::uint64_t epoch_{};
  std::uint64_t dropped_{};
};
}
