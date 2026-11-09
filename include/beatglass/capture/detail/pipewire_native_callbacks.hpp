#pragma once

#include "beatglass/capture/pipewire_source.hpp"

namespace beatglass::pipewire::detail {
/** Bounded callback binding used by the eventual Linux pw_stream adapter. */
struct NativeCallbackBinding {
  MonitorStream* handoff{};
  bool process(std::span<const float> samples, std::uint64_t host_time,
               bool host_time_valid, double source_sample_time,
               bool sample_time_valid) const noexcept {
    return handoff && handoff->on_process(samples, host_time, host_time_valid,
                                           source_sample_time, sample_time_valid);
  }
  void error(int error_code) const noexcept { if (handoff) handoff->on_error(error_code); }
};
}
