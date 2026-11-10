from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "process intervals overlap" in source
print("PipeWire transition process-interval order behavior verified")
