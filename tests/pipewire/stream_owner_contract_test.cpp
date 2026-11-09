#include <catch2/catch_test_macros.hpp>
#include "beatglass/capture/detail/pipewire_stream_owner.hpp"

namespace {
class FakeOwner final : public beatglass::pipewire::detail::StreamOwner {
public:
  bool connect(const Config& config, beatglass::pipewire::MonitorStream&) override {
    last = config; active = !config.serial.empty() && config.sample_rate != 0 && config.channels != 0;
    return active;
  }
  bool connected() const noexcept override { return active; }
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
  CHECK(owner.last.serial == "sink-7");
  CHECK(owner.last.sample_rate == 48000);
  CHECK(owner.last.channels == 2);
  owner.stop();
  CHECK_FALSE(owner.connected());
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
