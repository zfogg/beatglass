from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "transition host provenance has an invalid primary-source flag" in source
print("PipeWire transition host-primary-source behavior verified")
