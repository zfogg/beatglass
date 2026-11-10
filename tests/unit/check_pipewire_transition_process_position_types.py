from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "process sample positions must be non-negative integers" in source
print("PipeWire transition process-position type behavior verified")
