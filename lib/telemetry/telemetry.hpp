#pragma once

#include <memory>
#include <cstdint>
#include <string_view>

namespace beatglass::telemetry {

/**
 * Owns the optional Gotel host on the non-audio application thread. Construction
 * reads OTEL_* configuration and creates no telemetry when the feature is not
 * compiled. Scopes are short-lived and must not be used from a capture callback.
 */
class Host {
public:
  Host();
  ~Host();
  Host(const Host&) = delete;
  Host& operator=(const Host&) = delete;

  /** Starts a bounded application operation; returns false if sampling drops it. */
  bool start(std::string_view name);
  bool stage(std::string_view name);
  void set(std::string_view key, std::string_view value);
  void set(std::string_view key, std::int64_t value);
  void metric(std::string_view name, double value);
  void log(std::string_view message, bool error = false);
  void fail(std::string_view type, std::string_view message);
  /** Completes the current operation successfully and drains pending exports. */
  void finish();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace beatglass::telemetry
