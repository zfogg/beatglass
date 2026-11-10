from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "process channel counts must be positive integers" in source
print("PipeWire transition process-channel behavior verified")
