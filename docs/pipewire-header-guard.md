# PipeWire header guard

The experimental target now checks for `pipewire/pipewire.h` before enabling
the native implementation and defines `BEATGLASS_HAS_PIPEWIRE=1` only when the
header and pkg-config dependency are present. This keeps future `pw_stream`
code explicitly guarded and fails configuration with a concrete dependency
message instead of producing a partial build.
