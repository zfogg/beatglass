#pragma once
#include "beatglass/capture/coreaudio_tap_internal.hpp"
#include "beatglass/capture/source.hpp"
#include "beatglass/capture/source_lifecycle.hpp"
#include "beatglass/core/audio_clock.hpp"
#include <memory>
#include <optional>

namespace beatglass::coreaudio {
/** Private control-thread backend seam. Implementations may allocate and are never called by the audio IOProc. The caller owns the backend and must stop it before destruction. */
class SourceBackend {
public:
  virtual ~SourceBackend() = default;
  /** Enumerate output sources or return a typed error. */
  virtual EnumerationResult enumerate() = 0;
  /** Start the requested output using the coordinator-owned epoch; false leaves a diagnostic in status(). */
  virtual bool start(const std::string& uid, std::uint64_t epoch) = 0;
  /** Return the format installed for the current epoch, absent until startup succeeds. */
  virtual std::optional<AudioFormat> format() const = 0;
  /** Poll control-thread health; failure must remain observable until stopped. */
  virtual TapStatus status() = 0;
  /** Copy a queued block to caller-owned storage. */
  virtual bool pop(CaptureBlock& block) = 0;
  // Documented manually in the API reference due to Hawkmoth pure virtual noexcept parsing.
  virtual bool stop() noexcept = 0;
  // Documented manually in the API reference due to Hawkmoth pure virtual noexcept parsing.
  virtual std::optional<std::int64_t> host_time_ns(std::uint64_t ticks) const noexcept = 0;
};

/** Ordered control notification, delivered before any data of a newly running epoch. A running event owns its format; invalidations have no epoch or format. */
struct SourceEvent {
  SourceAttempt attempt{};
  std::uint64_t sequence{};
  SourceState state{SourceState::stopped};
  std::optional<std::uint64_t> epoch;
  std::optional<AudioFormat> format;
  SourceStatus status;
};

/** Provisional serialized Core Audio source adapter. One owner calls every method on a non-audio control thread. The process-level epoch allocator must outlive this object and be shared across source recreation. Backend IOProc only copies PCM; pump calls the borrowed-block consumer synchronously on the owner. Take the running event and install its format/reset boundary before pumping; after an interruption/error event invalidate analysis and stop/restart with a fresh epoch. This polling interface has no cross-thread notification queue and does not revoke previously delivered results. */
class SourceAdapter final {
public:
  /** Take exclusive ownership of a backend; allocator remains borrowed. */
  SourceAdapter(SourceEpochAllocator& epochs, std::unique_ptr<SourceBackend> backend);
  /** Stop and quiesce on the owner thread. */
  ~SourceAdapter();
  /** Enumerate through the backend. */
  EnumerationResult enumerate();
  /** Start synchronously and return an attempt or typed failure. An accepted attempt has a pending running event; the consumer must remain safe for callback restrictions. */
  SourceStartResult start(const std::string& id, AudioSource::Consumer consumer);
  /** Invalidate the attempt and attempt to quiesce the backend. False reports an OS stop failure; this provisional adapter is not yet an AudioSource because that shared contract cannot express failed quiescence. */
  bool stop() noexcept;
  /** Return the last control-thread status; call take_event or pump regularly to poll backend health. */
  SourceStatus status() const;
  /** Return the next causal event once; subsequent calls poll native health. Event loss by the caller is not recoverable and requires stop/restart. */
  std::optional<SourceEvent> take_event();
  /** Confirm that the running event's format and reset boundary have been installed. Reject a stale or mismatched attempt/sequence. */
  bool acknowledge_running(SourceAttempt attempt, std::uint64_t sequence) noexcept;
  /** Deliver at most one borrowed PCM block after the running event was taken. Returns false for no data or an event requiring handling. Unknown or unrepresentable host time fails closed rather than being encoded as zero. */
  bool pump();
  /** Anchor for the most recently delivered block; absent before delivery or after interruption/stop. Uncertainty and latency are unknown. */
  std::optional<AudioClockAnchor> latest_anchor() const noexcept;
private:
  void interrupt(const TapStatus& status);
  SourceEpochAllocator& epochs_;
  std::unique_ptr<SourceBackend> backend_;
  SourceLifecycle lifecycle_;
  AudioSource::Consumer consumer_;
  SourceStatus status_;
  std::optional<SourceAttempt> attempt_;
  std::optional<SourceEvent> pending_;
  std::optional<AudioFormat> format_;
  std::optional<AudioClockAnchor> anchor_;
  CaptureBlock block_;
  std::uint64_t sequence_{};
  bool running_taken_{}, running_acknowledged_{}, quiescence_failed_{};
};

/** Create a native Core Audio backend on macOS. The returned object is owned by the caller and is not a stable or installed API. */
std::unique_ptr<SourceBackend> make_native_backend();
}
