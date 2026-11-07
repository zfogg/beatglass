#pragma once
#include "beatglass/core/visualization.hpp"
#include <cstdint>
#include <memory>
#include <optional>

class QAudioBufferOutput;

/** One provisional playback PCM visualization generation, owned on the Qt control thread.
 * Construct a new object for every source or seek generation using the application's
 * nonzero epoch. Attach output() to that generation's QMediaPlayer. Retire the old
 * generation before changing source or seeking; do not retag a shared decoder's
 * late buffers. The owner must stop/detach and quiesce the player before destroying
 * this object or its Qt output. No method is an audio-device capture callback.
 * Qt 6.8 supports this tap only with its FFmpeg backend; unsupported backends can
 * emit no PCM. This is decoded player data, not an audible-output clock or file
 * identity proof. Frame positions are counts from the first accepted buffer in
 * this generation and are not a track-relative seek position. Timestamps must be
 * present, at most seven days, and contiguous to within two microseconds; otherwise it fails closed.
 * Every publication has a strictly increasing revision; terminal invalidation
 * reserves the maximum revision and never wraps. The UI must reject stale
 * epoch/revision pairs, recheck isCurrent() before painting cached output,
 * and clear the frame on invalidation. This check is momentary: a concurrent
 * invalidation cannot revoke an already painted or in-flight frame. The UI
 * must serialize its acceptance and clearing with painting, poll status even
 * when motion is reduced, and arrange a subsequent clear; scheduling means
 * no hard wall-clock bound is guaranteed.
 */
class PlaybackVisualization {
public:
  /** Terminal status; any status other than active requires a new generation. */
  enum class Status : unsigned { active, retired, end_of_stream, unsupported_audio, overflow };
  /** An owned frame and its strictly increasing, generation-local publication revision. */
  struct Frame {
    /** PCM-derived waveform and spectrum. */ beatglass::VisualizationFrame value;
    /** Nonzero revision; the maximum unsigned value is reserved for invalidation. */ unsigned revision{};
  };
  /** Create a bounded preallocated tap and worker; throws for zero epoch or allocation failure. */
  explicit PlaybackVisualization(std::uint64_t epoch);
  /** Stop and join the worker after the caller has quiesced the player; control thread only. */
  ~PlaybackVisualization();
  PlaybackVisualization(const PlaybackVisualization&) = delete;
  PlaybackVisualization& operator=(const PlaybackVisualization&) = delete;
  /** Borrow the Qt-owned output for this generation; control thread only. */
  QAudioBufferOutput* output() const noexcept;
  /** Invalidate this generation synchronously; call before seek, source change or stop. */
  void retire() noexcept;
  /** Return current status. An inactive generation never becomes active again. */
  Status status() const noexcept;
  /** Return the most recent owned frame once; inactive generations return empty. Control thread only. */
  std::optional<Frame> takeLatest();
  /** Copy the latest publication revision, or the reserved maximum once terminal. */
  unsigned revision() const noexcept;
  /** Momentary frame epoch/revision check; the caller must serialize presentation and arrange a later clear after invalidation. */
  bool isCurrent(std::uint64_t epoch, unsigned revision) const noexcept;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
