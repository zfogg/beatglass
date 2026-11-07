#include "beatglass/capture/coreaudio_source_internal.hpp"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <utility>
using namespace beatglass;
using namespace beatglass::coreaudio;

namespace {
class FakeBackend final : public SourceBackend {
public:
  TapStatus native{TapState::stopped, {}, {}};
  CaptureBlock block{};
  bool available{}, stop_ok{true}, start_ok{true}, interrupt_after_pop{}, throw_start{}, throw_status{};
  std::uint64_t epoch{};
  EnumerationResult enumerate() override { return {EnumerationOutcome::ok, {{"output", "Output", true, false}}, {}, {}}; }
  bool start(const std::string&, std::uint64_t e) override {
    epoch = e;
    if (throw_start) throw 1;
    if (!start_ok) { native = {TapState::error, "denied", TapFailure::permission_denied}; return false; }
    native = {TapState::running, {}, {}}; return true;
  }
  std::optional<AudioFormat> format() const override { return AudioFormat{48000, 2, ChannelLayout::stereo}; }
  TapStatus status() override { if (throw_status) throw 1; return native; }
  bool pop(CaptureBlock& out) override {
    if (!available) return false;
    out = block; available = false;
    if (interrupt_after_pop) native = {TapState::interrupted, "device changed during dequeue", TapFailure::device_lost};
    return true;
  }
  bool stop() noexcept override { if (stop_ok) native.state = TapState::stopped; return stop_ok; }
  std::optional<std::int64_t> host_time_ns(std::uint64_t ticks) const noexcept override { return static_cast<std::int64_t>(ticks); }
  void prepare(bool valid = true) {
    block = {}; block.epoch = epoch; block.sample_rate = 48000; block.channels = 2;
    block.sample_count = 2; block.samples[0] = 0.5f; block.host_time = 42; block.host_time_valid = valid; available = true;
  }
};
}

TEST_CASE("Core Audio control installs each fresh epoch before PCM", "[coreaudio]") {
  SourceEpochAllocator epochs;
  auto fake = std::make_unique<FakeBackend>(); auto* backend = fake.get();
  SourceAdapter source(epochs, std::move(fake));
  int calls = 0; std::uint64_t seen_epoch = 0;
  const auto consumer = [&](const AudioBlock& block) { ++calls; seen_epoch = block.epoch; CHECK(block.samples[0] == 0.5f); };
  const auto started = source.start("output", consumer); REQUIRE(started.attempt); REQUIRE_FALSE(started.error);
  backend->prepare(); CHECK_FALSE(source.pump()); CHECK(calls == 0);
  const auto running = source.take_event(); REQUIRE(running); CHECK(running->sequence == 1); REQUIRE(running->epoch);
  REQUIRE(running->format); CHECK(running->format->sample_rate == 48000);
  CHECK_FALSE(source.acknowledge_running({started.attempt->generation + 1}, 1));
  REQUIRE(source.acknowledge_running(*started.attempt, 1));
  REQUIRE(source.pump()); CHECK(calls == 1); CHECK(seen_epoch == *running->epoch);
  REQUIRE(source.latest_anchor()); CHECK(source.latest_anchor()->monotonic_ns == 42);
  CHECK(source.latest_anchor()->domain == AudioClockDomain::backend_monotonic);
  backend->native = {TapState::interrupted, "device changed", TapFailure::device_lost};
  CHECK_FALSE(source.pump()); CHECK_FALSE(source.latest_anchor());
  const auto lost = source.take_event(); REQUIRE(lost); CHECK(lost->sequence == 2);
  CHECK(lost->state == SourceState::interrupted); CHECK(lost->status.error == SourceError::device_lost);
  CHECK_FALSE(source.pump()); CHECK(source.stop());
  const auto next = source.start("output", consumer); REQUIRE(next.attempt);
  const auto restarted = source.take_event(); REQUIRE(restarted); REQUIRE(restarted->epoch);
  CHECK(*restarted->epoch > *running->epoch);
  CHECK_FALSE(source.acknowledge_running(*started.attempt, 1));
}

TEST_CASE("Core Audio control fails closed on absent time, stale PCM and stop failure", "[coreaudio]") {
  SourceEpochAllocator epochs;
  auto fake = std::make_unique<FakeBackend>(); auto* backend = fake.get();
  SourceAdapter source(epochs, std::move(fake)); int calls = 0;
  auto started = source.start("output", [&](const AudioBlock&) { ++calls; }); REQUIRE(started.attempt);
  REQUIRE(source.take_event()); REQUIRE(source.acknowledge_running(*started.attempt, 1));
  backend->prepare(false); CHECK_FALSE(source.pump()); CHECK(calls == 0);
  auto lost = source.take_event(); REQUIRE(lost); CHECK(lost->state == SourceState::interrupted);
  CHECK_FALSE(source.latest_anchor()); CHECK(source.stop());
  started = source.start("output", [&](const AudioBlock&) { ++calls; }); REQUIRE(started.attempt);
  REQUIRE(source.take_event()); REQUIRE(source.acknowledge_running(*started.attempt, 1));
  backend->prepare(); --backend->block.epoch; CHECK_FALSE(source.pump()); CHECK(calls == 0);
  REQUIRE(source.take_event()); backend->stop_ok = false; CHECK_FALSE(source.stop());
  CHECK(source.status().state == SourceState::error);
  CHECK_FALSE(source.start("output", [](const AudioBlock&) {}).attempt);
  backend->stop_ok = true; CHECK(source.stop());
  CHECK(source.start("output", [](const AudioBlock&) {}).attempt);
}

TEST_CASE("Core Audio control maps only explicit permission denial", "[coreaudio]") {
  SourceEpochAllocator epochs;
  auto fake = std::make_unique<FakeBackend>(); fake->start_ok = false;
  SourceAdapter source(epochs, std::move(fake));
  const auto result = source.start("output", [](const AudioBlock&) {});
  CHECK_FALSE(result.attempt); CHECK(result.error == SourceError::permission_denied);
  CHECK(source.status().error == SourceError::permission_denied);
}

TEST_CASE("Core Audio control invalidates a throwing consumer", "[coreaudio]") {
  SourceEpochAllocator epochs;
  auto fake = std::make_unique<FakeBackend>(); auto* backend = fake.get();
  SourceAdapter source(epochs, std::move(fake));
  const auto started = source.start("output", [](const AudioBlock&) { throw 1; }); REQUIRE(started.attempt);
  REQUIRE(source.take_event()); REQUIRE(source.acknowledge_running(*started.attempt, 1));
  backend->prepare(); CHECK_FALSE(source.pump()); CHECK_FALSE(source.latest_anchor());
  const auto event = source.take_event(); REQUIRE(event);
  CHECK(event->state == SourceState::interrupted); CHECK(event->sequence == 2);
  CHECK_FALSE(source.pump());
}

TEST_CASE("Core Audio control drops PCM when the device changes during dequeue", "[coreaudio]") {
  SourceEpochAllocator epochs;
  auto fake = std::make_unique<FakeBackend>(); auto* backend = fake.get();
  SourceAdapter source(epochs, std::move(fake)); int calls = 0;
  const auto started = source.start("output", [&](const AudioBlock&) { ++calls; }); REQUIRE(started.attempt);
  REQUIRE(source.take_event()); REQUIRE(source.acknowledge_running(*started.attempt, 1));
  backend->prepare(); backend->interrupt_after_pop = true;
  CHECK_FALSE(source.pump()); CHECK(calls == 0); CHECK_FALSE(source.latest_anchor());
  const auto event = source.take_event(); REQUIRE(event);
  CHECK(event->state == SourceState::interrupted); CHECK(event->sequence == 2);
  CHECK(event->status.error == SourceError::device_lost);
}

TEST_CASE("Core Audio control fails closed on backend exceptions", "[coreaudio]") {
  SourceEpochAllocator epochs;
  auto fake = std::make_unique<FakeBackend>(); auto* backend = fake.get();
  SourceAdapter source(epochs, std::move(fake));
  backend->throw_start = true;
  const auto failed = source.start("output", [](const AudioBlock&) {});
  CHECK_FALSE(failed.attempt); CHECK(failed.error == SourceError::backend_error);
  CHECK(source.status().state == SourceState::error);
  backend->throw_start = false;
  int calls = 0;
  const auto started = source.start("output", [&](const AudioBlock&) { ++calls; }); REQUIRE(started.attempt);
  REQUIRE(source.take_event()); REQUIRE(source.acknowledge_running(*started.attempt, 1));
  backend->prepare(); backend->throw_status = true;
  CHECK_FALSE(source.pump()); CHECK(calls == 0); CHECK_FALSE(source.latest_anchor());
  const auto event = source.take_event(); REQUIRE(event);
  CHECK(event->state == SourceState::interrupted); CHECK(event->status.error == SourceError::backend_error);
}

TEST_CASE("Core Audio consumer cannot reenter delivery or stop its source", "[coreaudio]") {
  SourceEpochAllocator epochs;
  auto fake = std::make_unique<FakeBackend>(); auto* backend = fake.get();
  SourceAdapter source(epochs, std::move(fake)); int calls = 0;
  const auto started = source.start("output", [&](const AudioBlock& block) {
    ++calls;
    CHECK_FALSE(source.pump());
    CHECK_FALSE(source.stop());
    CHECK_FALSE(source.take_event());
    CHECK(block.samples[0] == 0.5f);
  });
  REQUIRE(started.attempt); REQUIRE(source.take_event());
  REQUIRE(source.acknowledge_running(*started.attempt, 1));
  backend->prepare(); REQUIRE(source.pump()); CHECK(calls == 1);
  CHECK(source.status().state == SourceState::running);
  CHECK(source.stop());
}
