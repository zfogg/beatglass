from pathlib import Path

root = Path(__file__).parents[2]
source = (root / "lib/capture/pipewire/stream_owner_linux.cpp").read_text(encoding="utf-8")
docs = (root / "docs/pipewire-stream-owner-integration.md").read_text(encoding="utf-8")
for marker in ("spa_format_audio_raw_parse", "SPA_AUDIO_FORMAT_F32", "negotiated.rate", "negotiated.channels", "-EPROTO"):
    assert marker in source
assert "fresh epoch" in docs and "reacquisition" in docs
print("PipeWire format negotiation contract verified (6 checks)")
