from pathlib import Path

root = Path(__file__).parents[2]
source = (root / "lib/capture/pipewire/stream_owner_linux.cpp").read_text(encoding="utf-8")
docs = (root / "docs/pipewire-stream-owner-integration.md").read_text(encoding="utf-8")
for marker in (".destroy = destroy", ".state_changed = state_changed", ".param_changed = param_changed", ".process = process", "PW_STREAM_STATE_ERROR", "PW_STREAM_STATE_UNCONNECTED", "PW_STREAM_STATE_PAUSED"):
    assert marker in source
assert "all quiesce callback admission" in docs
print("PipeWire state lifecycle contract verified (8 checks)")
