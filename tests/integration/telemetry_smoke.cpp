#include "telemetry.hpp"
#include <cstdint>

int main() {
  beatglass::telemetry::Host telemetry;
  if (!telemetry.start("beatglass.analyze")) return 2;
  telemetry.set("audio.format", "smoke");
  telemetry.set("audio.sample_rate", static_cast<std::int64_t>(44100));
  telemetry.set("audio.channels", static_cast<std::int64_t>(2));
  telemetry.set("audio.frames", static_cast<std::int64_t>(512));
  telemetry.metric("beatglass.operations", 1);
  telemetry.metric("beatglass.frames", 512);
  telemetry.metric("beatglass.beats", 1);
  telemetry.log("beatglass telemetry smoke finished");
  telemetry.finish();
  return 0;
}
