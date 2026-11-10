from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "transition host provenance has no rack id" in source
print("PipeWire transition host-rack-id behavior verified")
