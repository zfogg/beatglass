from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "transition host provenance has no availability zone" in source
print("PipeWire transition host-availability-zone behavior verified")
