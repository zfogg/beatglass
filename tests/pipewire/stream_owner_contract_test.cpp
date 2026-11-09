#include <catch2/catch_test_macros.hpp>
#include "beatglass/capture/detail/pipewire_stream_owner.hpp"
#include <array>

namespace {
class FakeOwner final : public beatglass::pipewire::detail::StreamOwner {
public:
  bool connect(const Config& config, beatglass::pipewire::MonitorStream&) override {
    if (active) return false;
    last = config; active = !config.serial.empty() && config.sample_rate != 0 && config.channels != 0;
    return active;
  }
  bool connected() const noexcept override { return active; }
  bool process(std::span<const float> samples, std::uint64_t host_time, bool host_time_valid,
               double source_sample_time, bool sample_time_valid,
               bool discontinuity,
               beatglass::pipewire::MonitorStream& handoff) noexcept override {
    return active && handoff.push(samples, host_time, host_time_valid, discontinuity,
                                  source_sample_time, sample_time_valid);
  }
  void report_error(int error_code, beatglass::pipewire::MonitorStream& handoff) noexcept override {
    handoff.on_error(error_code);
  }
  void stop() noexcept override { active = false; }
  Config last;
  bool active = false;
};
}

TEST_CASE("stream owner contract preserves target and negotiated format lifecycle") {
  FakeOwner owner;
  beatglass::pipewire::MonitorStream handoff(7, 48000, 2);
  REQUIRE(owner.connect({"sink-7", 48000, 2}, handoff));
  CHECK(owner.connected());
  CHECK(owner.process(std::array<float, 2>{0.25F, -0.25F}, 42, true, 12.5, true, false, handoff));
  CHECK(owner.last.serial == "sink-7");
  CHECK(owner.last.sample_rate == 48000);
  CHECK(owner.last.channels == 2);
  owner.stop();
  CHECK_FALSE(owner.connected());
  owner.report_error(-32, handoff);
  CHECK(handoff.needs_restart());
  REQUIRE(owner.connect({"sink-8", 44100, 2}, handoff));
  CHECK(owner.connected());
}

TEST_CASE("stream owner rejects incomplete native stream configuration") {
  FakeOwner owner;
  beatglass::pipewire::MonitorStream handoff(8, 48000, 2);
  CHECK_FALSE(owner.connect({"", 48000, 2}, handoff));
  CHECK_FALSE(owner.connected());
  CHECK_FALSE(owner.connect({"sink-8", 0, 2}, handoff));
  CHECK_FALSE(owner.connected());
  owner.stop();
  CHECK_FALSE(owner.connected());
}
