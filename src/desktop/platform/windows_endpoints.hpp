#pragma once
#include <string>
#include <vector>

namespace beatglass::windows {
/** A currently active render endpoint. IDs are opaque UTF-8 Windows endpoint IDs; the name is for display and need not be unique. */
struct RenderEndpoint {
  std::string id;
  std::string name;
  bool is_default{};
};

/** Result of endpoint discovery. An empty list with no error is valid. An error contains a diagnostic and the list must not be treated as complete. */
struct EndpointResult {
  std::vector<RenderEndpoint> endpoints;
  std::string error;
};

/** Enumerate active Windows render endpoints and mark the multimedia default. Call from a non-audio thread; this initializes COM on that thread, retains no interfaces, and returns owned values. A concurrent device change can make the snapshot stale. Failures are returned as diagnostics. */
EndpointResult enumerate_render_endpoints();
}
