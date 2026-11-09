#include <catch2/catch_test_macros.hpp>
#include "beatglass/capture/pipewire_source.hpp"
#include <cstdlib>

TEST_CASE("PipeWire source adapter rejects invalid start without active context") {
  auto backend = beatglass::pipewire::make_monitor_backend();
  REQUIRE(backend);
  CHECK_FALSE(backend->start_stream("", 1, 48000, 2));
  const auto status = backend->poll_status();
  CHECK_FALSE(status.active);
  CHECK(status.requires_reacquisition());
  CHECK_FALSE(status.has_context());
}

TEST_CASE("PipeWire source adapter fails closed for an unknown target") {
  auto backend = beatglass::pipewire::make_monitor_backend();
  REQUIRE(backend);
  REQUIRE(setenv("PIPEWIRE_REMOTE", "beatglass-test-no-such-remote", 1) == 0);
  CHECK_FALSE(backend->start_stream("__beatglass_unknown_target__", 42, 48000, 2));
  unsetenv("PIPEWIRE_REMOTE");
  CHECK_FALSE(backend->selected_active());
  CHECK(backend->poll_status().requires_reacquisition());
  CHECK_FALSE(backend->reacquire("__beatglass_unknown_target__", 43, 48000, 2));
  CHECK_FALSE(backend->selected_active());
}
