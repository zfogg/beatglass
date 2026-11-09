#pragma once

#include "beatglass/capture/detail/pipewire_discovery.hpp"
#include "beatglass/capture/coreaudio_fifo_internal.hpp"
#include <chrono>
#include <cstdint>
#include <memory>
#include <span>
#include <atomic>
#include <string>
#include <optional>

namespace beatglass::pipewire {
struct MonitorFormat {
  std::uint32_t sample_rate{};
  std::uint32_t channels{};
};
struct MonitorStatus {
  bool active{};
  bool needs_restart{};
  bool discontinuity{};
  bool overflowed{};
  int error_code{};
  std::string serial;
  std::optional<MonitorFormat> format;
  std::uint64_t epoch{};
};
/** Control-thread lifecycle for a future PipeWire output-monitor stream.
 * Implementations own the PipeWire stream and never expose callback memory;
 * capture callbacks copy bounded PCM into caller-owned storage. A source
 * switch, disconnect, format change, or overflow must publish discontinuity
 * and require a fresh nonzero epoch before data is accepted.
 */
class MonitorBackend {
public:
  virtual ~MonitorBackend() = default;
  /** Enumerate output sinks with a bounded control-thread timeout. */
  virtual Discovery enumerate(std::chrono::milliseconds timeout) = 0;
  /** Select a sink and begin a fresh epoch; false leaves status observable. */
  virtual bool start(const std::string& serial, std::uint64_t epoch) = 0;
  /** Start a stream after the negotiated native format is known. */
  virtual bool start_stream(const std::string& serial, std::uint64_t epoch,
                            std::uint32_t sample_rate, std::uint32_t channels) = 0;
  /** Restart the selected stream with a fresh epoch and negotiated format. */
  virtual bool restart_stream(std::uint64_t epoch, std::uint32_t sample_rate,
                              std::uint32_t channels) = 0;
  /** Return the currently selected sink serial, or an empty value when stopped. */
  virtual std::string selected_serial() const = 0;
  /** Return the negotiated format for the selected stream, if active. */
  virtual std::optional<MonitorFormat> selected_format() const noexcept = 0;
  /** Return the epoch currently owned by the selected session, or zero when stopped. */
  virtual std::uint64_t selected_epoch() const noexcept = 0;
  /** Return whether the selected stream is currently accepting callback data. */
  virtual bool selected_active() const noexcept = 0;
  /** Return whether a block tagged with epoch belongs to the selected stream. */
  virtual bool accepts_epoch(std::uint64_t epoch) const noexcept = 0;
  /** Pop one bounded block on the control thread after start_stream succeeds. */
  virtual bool pop(coreaudio::CaptureBlock& block) noexcept = 0;
  virtual MonitorStatus poll_status() noexcept = 0;
  /** Stop and quiesce the stream; safe to call repeatedly. */
  virtual bool stop() noexcept = 0;
  /** Release the selected session and negotiated format on the control thread. */
  virtual void reset() noexcept = 0;
};
/** Bounded callback handoff used by a PipeWire stream adapter. The callback only calls push; control code calls pop/stop. */
class MonitorStream {
public:
  MonitorStream(std::uint64_t epoch, std::uint32_t sample_rate, std::uint32_t channels) noexcept;
  /** Copy one callback block into preallocated storage; false records overflow/discontinuity. */
  bool push(std::span<const float> samples, std::uint64_t host_time, bool host_time_valid,
            bool discontinuity, double source_sample_time, bool sample_time_valid) noexcept;
  /** Native pw_stream process callback seam; marks a discontinuity before the next block when requested. */
  bool on_process(std::span<const float> samples, std::uint64_t host_time, bool host_time_valid,
                  double source_sample_time, bool sample_time_valid) noexcept;
  /** Native state/error callback seam; closes callback admission until control-thread restart. */
  void on_error(int error_code) noexcept;
  /** Read the last native callback error code on the control thread; zero means none. */
  int error_code() const noexcept;
  /** Consume and clear the last callback error on the control thread. */
  int take_error() noexcept;
  /** Consume and clear the pending discontinuity marker on the control thread. */
  bool take_discontinuity() noexcept;
  /** Control-thread predicate: a callback error or stop requires a fresh restart epoch. */
  bool needs_restart() const noexcept;
  /** Control-thread restart boundary: replace the FIFO with a fresh epoch and format. */
  bool restart(std::uint64_t epoch, std::uint32_t sample_rate, std::uint32_t channels);
  /** Pop the next bounded block on the control thread. */
  bool pop(coreaudio::CaptureBlock& block) noexcept;
  /** Stop accepting callback data and expose overflow state. */
  void stop() noexcept;
  bool overflowed() const noexcept;
private:
  std::unique_ptr<coreaudio::CaptureFifo> fifo_;
  std::atomic<bool> stopped_{false};
  std::atomic<bool> discontinuity_pending_{false};
  std::atomic<int> error_code_{0};
};
/** Control-thread owner for one selected output monitor epoch. */
class MonitorSession {
public:
  using Format = MonitorFormat;
  using Status = MonitorStatus;
  /** Select a validated sink and allocate its bounded stream on the control thread. */
  bool start(std::string serial, std::uint64_t epoch, std::uint32_t sample_rate,
             std::uint32_t channels);
  /** Stop the active stream and publish a discontinuity before quiescing callbacks. */
  void stop() noexcept;
  /** Stop and release the selected stream, identity, and format on the control thread. */
  void reset() noexcept;
  /** True when no stream exists or its callback state requires reacquisition. */
  bool needs_restart() const noexcept;
  /** Control-thread restart with a fresh epoch and format. */
  bool restart(std::uint64_t epoch, std::uint32_t sample_rate, std::uint32_t channels);
  /** Return the format installed for the active epoch, if any. */
  std::optional<Format> format() const noexcept { return format_; }
  /** Native process seam for the currently selected stream. */
  bool on_process(std::span<const float> samples, std::uint64_t host_time,
                  bool host_time_valid, double source_sample_time,
                  bool sample_time_valid) noexcept;
  /** Pop one bounded PCM block on the control thread; callback memory never escapes. */
  bool pop(coreaudio::CaptureBlock& block) noexcept;
  /** Consume callback error/discontinuity markers for one control-thread poll. */
  Status poll_status() noexcept;
  const std::string& serial() const noexcept { return serial_; }
  std::uint64_t epoch() const noexcept { return epoch_; }
  bool accepts_epoch(std::uint64_t epoch) const noexcept {
    return epoch != 0 && epoch == epoch_ && stream_ && !stream_->needs_restart();
  }
  bool active() const noexcept { return stream_ && !stream_->needs_restart(); }
private:
  std::string serial_;
  std::unique_ptr<MonitorStream> stream_;
  std::optional<Format> format_;
  std::uint64_t epoch_{};
};
/** Create the provisional PipeWire monitor boundary; ownership stays with the caller. */
std::unique_ptr<MonitorBackend> make_monitor_backend();
}
