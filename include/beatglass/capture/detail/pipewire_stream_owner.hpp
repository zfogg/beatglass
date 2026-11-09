#pragma once
#include "beatglass/capture/pipewire_source.hpp"
#include <memory>
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
  virtual void stop() noexcept = 0;
};
std::unique_ptr<StreamOwner> make_stream_owner();
}
