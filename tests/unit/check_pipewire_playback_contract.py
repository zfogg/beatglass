"""Static contract checks for the bounded Linux PipeWire playback session."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
header = (root / "include/beatglass/desktop/pipewire_playback_session.hpp").read_text(encoding="utf-8")
source = (root / "lib/capture/pipewire/playback_session_linux.cpp").read_text(encoding="utf-8")
tui = (root / "src/cli/cli_main.cpp").read_text(encoding="utf-8")
required = {
    "shared playback state": "beatglass/core/playback_session.hpp",
    "bounded queue": "PlaybackPcmQueue",
    "output direction": "PW_DIRECTION_OUTPUT",
    "mapped callback": "PW_STREAM_FLAG_MAP_BUFFERS",
    "callback dequeue": "self.queue.pop",
    "underrun silence": "std::fill_n(output, capacity, 0.0f)",
    "epoch admission": "block.epoch == session_.epoch()",
    "CLI wiring": "beatglass::pipewire::PlaybackSession output",
}
for label, marker in required.items():
    haystack = tui if label == "CLI wiring" else header + source
    if marker not in haystack:
        raise SystemExit(f"missing PipeWire playback contract: {label}")
print(f"PipeWire playback contract verified ({len(required)} checks)")
