#pragma once

#include "beatglass/capture/playback_output_adapter.hpp"
#include "beatglass/core/playback_session.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>

namespace beatglass::pipewire {

/**
 * Linux PipeWire playback session used by the CLI.
 *
 * The control thread owns the PipeWire lifecycle and PlaybackSession epoch.
 * submit() copies borrowed decoder PCM into a bounded SPSC queue. The native
 * process callback only dequeues a bounded block, copies it into PipeWire's
 * mapped buffer, and returns the buffer; it never allocates, blocks, or calls
 * UI/database code. A failed native stream is observable through status() and
 * requires a fresh start epoch.
 *
 * This class is available only when the PipeWire build is enabled.
 */
class PlaybackSession final {
public:
  PlaybackSession();
  ~PlaybackSession();
  PlaybackSession(const PlaybackSession&) = delete;
  PlaybackSession& operator=(const PlaybackSession&) = delete;

  /** Start a fresh native output stream at the default PipeWire target. */
  bool start(std::uint32_t sample_rate, std::uint16_t channels,
             std::size_t max_samples, std::uint64_t epoch, std::string& error);
  /** Copy one decoder block when the shared session is playing. */
  bool submit(const AudioBlock& block) noexcept;
  /** True when the native stream is actively accepting output. */
  bool active() const noexcept;
  /** Pause/resume the shared session and native stream transactionally. */
  bool pause() noexcept;
  bool resume() noexcept;
  /** Stop and quiesce the native stream; safe to call repeatedly. */
  void stop() noexcept;
  PlaybackState state() const noexcept { return session_.state(); }
  std::uint64_t epoch() const noexcept { return session_.epoch(); }
  std::string status() const;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
  beatglass::PlaybackSession session_;
};

} // namespace beatglass::pipewire
