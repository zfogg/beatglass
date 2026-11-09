#pragma once
#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace beatglass::pipewire {
/** Private, provisional PipeWire object identity; serials are server-session scoped. */
struct Sink { std::string serial, name, description; };
/** A snapshot or a nonempty diagnostic; an error never carries partial sinks.
 * `error_code` preserves the native PipeWire result for permission/recovery
 * policy while `error` remains suitable for operator-facing diagnostics. */
struct Discovery { std::vector<Sink> sinks; int error_code{}; std::string error; };
/** Internal registry accumulator. Own and mutate on one control thread only. */
class RegistrySnapshot {
public:
  /** Accept an addressable Audio/Sink node; properties may be null. */
  void add(std::uint32_t id, const char* type, const char* media_class, const char* serial,
           const char* name, const char* description);
  /** Remove a disappeared registry global. */
  void remove(std::uint32_t id);
  /** Copy the current snapshot on its owning control thread. */
  std::vector<Sink> sinks() const;
private:
  std::map<std::uint32_t, Sink> nodes_;
};
/**
 * Enumerate sinks on the calling control thread, up to timeout.
 * Success may be empty; errors include connection failures and timeouts.
 * The serial is valid only during the current PipeWire server session.
 * This provisional internal interface is not an installed API.
 */
Discovery discover_sinks(std::chrono::milliseconds timeout);
}
