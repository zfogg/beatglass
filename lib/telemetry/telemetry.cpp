#include "telemetry.hpp"

#ifdef BEATGLASS_HAS_GOTEL
#include <gotel/gotel.hpp>
#include <array>
#include <optional>
#include <array>
#include <chrono>
#include <string>

namespace beatglass::telemetry {
static std::uint64_t monotonic_ns() {
  return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count());
}
static std::uint64_t realtime_ns() {
  return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count());
}
struct Host::Impl {
  std::array<gotel_operation_t, 4> operations{{
      {"beatglass.analyze", false}, {"beatglass.replay", false},
      {"beatglass.decode", false}, {"beatglass.serialize", false}}};
  std::array<double, 5> duration_bounds{{1, 5, 10, 100, 1000}};
  std::array<gotel_metric_t, 6> metrics{{
      {"beatglass.operations", "{operation}", OTLP_METRIC_COUNTER, nullptr, 0},
      {"beatglass.errors", "{error}", OTLP_METRIC_COUNTER, nullptr, 0},
      {"beatglass.duration", "ms", OTLP_METRIC_HISTOGRAM, duration_bounds.data(), duration_bounds.size()},
      {"beatglass.frames", "frames", OTLP_METRIC_COUNTER, nullptr, 0},
      {"beatglass.beats", "beats", OTLP_METRIC_COUNTER, nullptr, 0},
      {"beatglass.bpm", "BPM", OTLP_METRIC_GAUGE, nullptr, 0}}};
  gotel_env_storage_t environment{};
  gotel_options_t options{};
  std::optional<gotel::host> host;
  std::optional<gotel::scope> scope;
  std::optional<gotel::scope> child;

  Impl() {
    options.signals = GOTEL_TRACES | GOTEL_LOGS | GOTEL_METRICS;
    options.operations = operations.data();
    options.operation_count = operations.size();
    options.metrics = metrics.data();
    options.metric_count = metrics.size();
    options.application_name = "beatglass";
    options.exporter.service_name = "beatglass";
    options.monotonic_ns = monotonic_ns;
    options.realtime_ns = realtime_ns;
    options.omit_command_args = true;
    (void)gotel_options_apply_env(&options, &environment, "http://127.0.0.1:4318");
    auto created = gotel::host::create(options);
    if (created) host.emplace(std::move(*created));
  }
};

Host::Host() : impl_(std::make_unique<Impl>()) {}
Host::~Host() = default;
bool Host::start(std::string_view name) {
  if (!impl_->host) return false;
  const auto index = name.find("replay") != std::string_view::npos ? 1u : 0u;
  auto started = impl_->host->start(index);
  if (started) impl_->scope.emplace(std::move(*started));
  if (impl_->scope) impl_->scope->set("beatglass.operation", name);
  return impl_->scope.has_value();
}
bool Host::stage(std::string_view name) {
  if (!impl_->host) return false;
  const auto index = name.find("decode") != std::string_view::npos ? 2u : 3u;
  auto started = impl_->host->start(index);
  if (started) impl_->child.emplace(std::move(*started));
  return impl_->child.has_value();
}
void Host::set(std::string_view key, std::string_view value) { if (impl_->scope) impl_->scope->set(key, value); }
void Host::set(std::string_view key, std::int64_t value) { if (impl_->scope) impl_->scope->set(key, value); }
void Host::metric(std::string_view name, double value) {
  if (!impl_->host) return;
  static constexpr std::array<std::string_view, 6> names{"beatglass.operations", "beatglass.errors", "beatglass.duration", "beatglass.frames", "beatglass.beats", "beatglass.bpm"};
  for (std::size_t i = 0; i < names.size(); ++i) if (names[i] == name) { (void)gotel_metric_record(impl_->host->native(), i, value); break; }
}
void Host::log(std::string_view message, bool error) {
  if (!impl_->host) return;
  const std::string bounded(message.substr(0, 240));
  gotel_log(impl_->host->native(), error ? OTLP_SEVERITY_ERROR : OTLP_SEVERITY_INFO, bounded.c_str());
}
void Host::fail(std::string_view type, std::string_view message) { if (impl_->scope) impl_->scope->fail(type, message); }
void Host::finish() {
  if (!impl_->scope) return;
  if (impl_->child) { impl_->child->success(); impl_->child.reset(); }
  impl_->scope->success();
  impl_->scope.reset();
  (void)impl_->host->tick();
  (void)impl_->host->drain();
}
} // namespace beatglass::telemetry
#else
namespace beatglass::telemetry {
struct Host::Impl {};
Host::Host() : impl_(std::make_unique<Impl>()) {}
Host::~Host() = default;
bool Host::start(std::string_view) { return false; }
bool Host::stage(std::string_view) { return false; }
void Host::set(std::string_view, std::string_view) {}
void Host::set(std::string_view, std::int64_t) {}
void Host::metric(std::string_view, double) {}
void Host::log(std::string_view, bool) {}
void Host::fail(std::string_view, std::string_view) {}
void Host::finish() {}
} // namespace beatglass::telemetry
#endif
