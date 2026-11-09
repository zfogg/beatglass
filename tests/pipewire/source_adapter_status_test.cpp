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

TEST_CASE("PipeWire monitor session requires a fresh epoch after callback failure") {
  beatglass::pipewire::MonitorSession session;
  REQUIRE(session.start("sink-a", 10, 48000, 2));
  const float samples[] = {0.1F, -0.1F};
  CHECK(session.on_process(samples, 100, true, 0.0, false));
  session.native_handoff()->on_error(-32);
  auto failed = session.poll_status();
  CHECK_FALSE(failed.active);
  CHECK(failed.needs_restart);
  CHECK(failed.has_error());
  CHECK(failed.discontinuity);
  CHECK_FALSE(session.restart(10, 48000, 2));
  REQUIRE(session.restart(11, 44100, 1));
  CHECK(session.epoch() == 11);
  REQUIRE(session.format());
  CHECK(session.format()->sample_rate == 44100);
  CHECK(session.format()->channels == 1);
  CHECK_FALSE(session.accepts_epoch(10));
  CHECK(session.accepts_epoch(11));
}
