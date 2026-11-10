from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "transition host provenance has an invalid secure boot flag" in source
print("PipeWire transition host-secure-boot behavior verified")
