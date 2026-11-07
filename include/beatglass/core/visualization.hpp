#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace beatglass {
/** One owned visualization window. Frame intervals are half-open within epoch. */
struct VisualizationFrame {
  /** Number of mono samples in an FFT window. */
  static constexpr unsigned window_size = 1024;
  /** Number of consecutive min/max waveform buckets. */
  static constexpr unsigned waveform_size = 64;
  /** Number of real FFT bins including DC and Nyquist. */
  static constexpr unsigned spectrum_size = window_size / 2 + 1;
  /** Source epoch and first inclusive / last exclusive frame positions. */
  std::uint64_t epoch{}, first_frame{}, end_frame{};
  /** Input sample rate in frames per second. */
  std::uint32_t sample_rate{};
  /** Minimum and maximum raw PCM value per consecutive 16-frame bucket. */
  std::array<float, waveform_size> minimum{}, maximum{};
  /** Linear single-sided peak amplitude, Hann-window corrected; bin k is k * sample_rate / 1024 Hz. Not dB or power. */
  std::array<float, spectrum_size> magnitude{};
};
/** Provisional fixed-memory mono visualization processor; not thread-safe.
 * Own on one worker thread; never call from capture callback or GUI thread.
 * No PCM view is retained. Windows are non-overlapping; a call may return only
 * its latest completed window, so callers must not interpret it as full history.
 * Reset, invalid input, or an epoch, rate, or frame discontinuity invokes the
 * invalidation hook synchronously and clears partial input. The owner must
 * gate every frame by the same shared generation and a monotonic revision;
 * epochs alone are insufficient, including for gaps within one epoch. On
 * source stop or before destroying the processor, its owner must synchronously
 * invalidate the shared publication state. Destruction does not revoke frames.
 */
class VisualizationProcessor {
public:
  /** Maximum accepted frames per call; callers split larger blocks on the worker. */
  static constexpr unsigned max_input_frames = 4096;
  /** Synchronous worker-thread invalidation hook. The owner advances the shared generation and clear revision before return; reject stale generations and revisions, synchronize visibility, and fail closed before counter overflow. The hook must not throw or reenter. */
  using Invalidate = void (*)(void*) noexcept;
  /** Construct with a required callback and borrowed context, valid for this object's lifetime.
   * @throws std::invalid_argument if the callback is null. */
  VisualizationProcessor(Invalidate invalidate, void* context);
  /** Invalidate published data synchronously and discard partial input. */
  void reset() noexcept;
  /** Consume contiguous finite normalized mono PCM in [-1, 1]. Input must have nonzero epoch and rate and at most 4096 frames,
   * and sample positions must not overflow. Invalid input invalidates and clears the processor before throwing.
   * Forward gaps and epoch changes discard the previous partial
   * window and begin a new one. Old epochs, same-epoch rate changes and overlaps are rejected
   * and invalidate output. Epoch and frame high-water marks survive reset; seeking
   * or restarting at an earlier frame requires a newer coordinator epoch. Out-of-range samples are rejected rather than clipped. Output owns its data.
   * @param epoch Coordinator-issued nonzero source epoch.
   * @param first_frame First source frame of the supplied span.
   * @param sample_rate Source sample rate in frames per second.
   * @param samples Borrowed mono PCM, valid only for this call.
   * @param discontinuity Explicit break even if frame positions remain contiguous.
   * @return Latest complete window in this call, or empty until one completes.
   * @throws std::invalid_argument for invalid metadata, out-of-range or nonfinite samples.
   */
  std::optional<VisualizationFrame> process(std::uint64_t epoch, std::uint64_t first_frame,
                                            std::uint32_t sample_rate, std::span<const float> samples,
                                            bool discontinuity = false);
private:
  void clear_partial() noexcept;
  Invalidate invalidate_;
  void* context_;
  std::array<float, VisualizationFrame::window_size> samples_{};
  std::uint64_t epoch_{}, next_frame_{}, window_start_{}, high_epoch_{}, high_next_{};
  std::uint32_t sample_rate_{}, high_rate_{};
  unsigned count_{};
};
}


