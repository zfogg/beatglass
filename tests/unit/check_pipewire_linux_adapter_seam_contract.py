from pathlib import Path

root = Path(__file__).parents[2]
source = (root / "lib/capture/pipewire/stream_owner_linux.cpp").read_text(encoding="utf-8")
docs = (root / "docs/pipewire-stream-owner-integration.md").read_text(encoding="utf-8")
assert "BEATGLASS_HAS_PIPEWIRE" in source
assert "pw_stream" in source
assert "SPA_PARAM_Format" in source or "SPA_PARAM_FORMAT" in source
assert "callback must dequeue" in source
for marker in ("sample rate/channel count", "target node", "bounded `MonitorStream`", "Linux native compilation"):
    assert marker in docs
print("PipeWire Linux adapter seam contract verified (7 checks)")
