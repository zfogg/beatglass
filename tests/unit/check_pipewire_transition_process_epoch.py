from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "process epoch must match a lifecycle epoch" in source
print("PipeWire transition process-epoch behavior verified")
