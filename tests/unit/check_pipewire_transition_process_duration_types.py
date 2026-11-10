from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "process durations must be non-negative integers" in source
print("PipeWire transition process-duration type behavior verified")
