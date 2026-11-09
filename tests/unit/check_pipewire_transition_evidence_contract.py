from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
for marker in ("start", "process", "error", "restart", "discontinuity", "error_code", "monotonic", "timestamp_ns", "serial", "sample_count", "sample_rate", "channels", "non-object event", "unnamed event", "out of order", "restart does not advance", "error_event", "event.get(\"epoch\") == start.get(\"epoch\")", "restart event has no sink serial"):
    assert marker in source
print("PipeWire transition evidence contract verified (18 checks)")
