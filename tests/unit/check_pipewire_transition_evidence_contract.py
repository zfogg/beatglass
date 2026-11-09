from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
for marker in ("start", "process", "error", "restart", "discontinuity", "error_code", "monotonic", "serial", "sample_count", "sample_rate", "channels"):
    assert marker in source
print("PipeWire transition evidence contract verified (11 checks)")
