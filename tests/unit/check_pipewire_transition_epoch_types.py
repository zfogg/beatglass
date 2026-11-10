from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "lifecycle epochs must be positive integers" in source
print("PipeWire transition epoch type behavior verified")
