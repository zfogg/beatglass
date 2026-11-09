#include <catch2/catch_test_macros.hpp>
#include "beatglass/capture/pipewire_source.hpp"

TEST_CASE("PipeWire source adapter rejects invalid start without active context") {
  auto backend = beatglass::pipewire::make_monitor_backend();
  REQUIRE(backend);
  CHECK_FALSE(backend->start_stream("", 1, 48000, 2));
  const auto status = backend->poll_status();
  CHECK_FALSE(status.active);
  CHECK(status.requires_reacquisition());
  CHECK_FALSE(status.has_context());
}
