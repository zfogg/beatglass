#pragma once
#include "beatglass/capture/pipewire_source.hpp"
#include <memory>
#include <span>
namespace beatglass::pipewire::detail {
/** Linux-only control-thread seam for native pw_stream ownership. */
class StreamOwner {
public:
  struct Config {
    std::string serial;
    std::uint32_t sample_rate{};
    std::uint32_t channels{};
  };
  virtual ~StreamOwner() = default;
  /** Control-thread connect contract; callback ownership remains with handoff. */
  virtual bool connect(const Config& config, MonitorStream& handoff) = 0;
  /** True while the native stream is connected and callback admission is valid. */
  virtual bool connected() const noexcept = 0;
  /** Native callback boundary; implementations copy bounded PCM into handoff. */
  virtual bool process(std::span<const float> samples, std::uint64_t host_time,
                       bool host_time_valid, double source_sample_time,
                       bool sample_time_valid, bool discontinuity,
                       MonitorStream& handoff) noexcept = 0;
  /** Native state/error callback boundary; closes callback admission. */
  virtual void report_error(int error_code, MonitorStream& handoff) noexcept = 0;
  virtual void stop() noexcept = 0;
};
std::unique_ptr<StreamOwner> make_stream_owner();
}
