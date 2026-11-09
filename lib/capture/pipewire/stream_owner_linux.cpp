// Linux-only PipeWire integration seam. Native construction remains provisional
// until a Linux host validates SPA negotiation and pw_stream callback timing.
#if defined(BEATGLASS_HAS_PIPEWIRE)
#include "beatglass/capture/detail/pipewire_stream_owner.hpp"
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/param.h>

namespace beatglass::pipewire::detail {
// This translation unit intentionally isolates PipeWire headers and the future
// pw_stream callback boundary from the portable StreamOwner contract. The
// callback must dequeue only bounded buffers and hand copied float PCM to
// MonitorStream; it must never allocate, block, or touch UI/database state.
static_assert(PW_VERSION_STREAM_EVENTS > 0, "PipeWire stream callback ABI required");
static_assert(SPA_PARAM_FORMAT > 0, "SPA format negotiation required");
}
#endif
