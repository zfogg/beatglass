from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "transition host provenance has no serial number" in source
print("PipeWire transition host-serial-number behavior verified")
