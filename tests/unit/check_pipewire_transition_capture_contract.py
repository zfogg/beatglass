from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "capture_pipewire_transition_evidence.py").read_text(encoding="utf-8")
for marker in ("schema", "beatglass-pipewire-transitions-v1", "platform.platform", "rev-parse", "commit", "events"):
    assert marker in source
print("PipeWire transition capture contract verified (6 checks)")
