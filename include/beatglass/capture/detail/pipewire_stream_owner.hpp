#pragma once
#include "beatglass/capture/pipewire_source.hpp"
#include <memory>
namespace beatglass::pipewire::detail {
/** Linux-only control-thread seam for native pw_stream ownership. */
class StreamOwner {
public:
  virtual ~StreamOwner() = default;
  virtual bool connect(const std::string& serial, std::uint32_t sample_rate,
                       std::uint32_t channels, MonitorStream& handoff) = 0;
  virtual void stop() noexcept = 0;
};
std::unique_ptr<StreamOwner> make_stream_owner();
}
