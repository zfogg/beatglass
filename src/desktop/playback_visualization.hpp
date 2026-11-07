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
 * This single-use generation has revision zero while active and one once
 * invalidated; it never wraps or becomes active again. The UI must recheck
 * isCurrent() before painting cached output and clear it on invalidation.
 */
class PlaybackVisualization {
public:
  /** Terminal status; any status other than active requires a new generation. */
  enum class Status : unsigned { active, retired, end_of_stream, unsupported_audio, overflow };
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
  /** Return the most recent owned frame once; all returned frames have revision zero. Inactive generations return empty. Control thread only. */
  std::optional<beatglass::VisualizationFrame> takeLatest();
  /** Copy the monotonic one-shot revision: zero while active, one once terminal. */
  unsigned revision() const noexcept;
  /** Recheck frame epoch and revision immediately before display; clear when false. */
  bool isCurrent(std::uint64_t epoch, unsigned revision = 0) const noexcept;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
