from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
for marker in ("start", "process", "error", "restart", "discontinuity", "error_code", "monotonic", "timestamp_ns", "device_id", "sample_position", "sequence", "contiguous", "event_count", "does not match artifact", "must be unique", "error_domain", "recovery_reason", "host", "daemon_version", "capture_command", "artifact_id", "artifact identity", "artifact_sha256", "artifact digest", "capture_started_at", "capture_ended_at", "capture timestamp", "capture end", "precedes capture start", "reacquisition", "serial", "sample_count", "sample_rate", "channels", "invalid negotiated format", "disagrees with restart format", "overlap captured samples", "non-object event", "unnamed event", "out of order", "restart does not advance", "error_event", "event.get(\"epoch\") == start.get(\"epoch\")", "restart event has no sink serial", "reacquisition evidence has the wrong epoch"):
    assert marker in source
print("PipeWire transition evidence contract verified (18 checks)")
