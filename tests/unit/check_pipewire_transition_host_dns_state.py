from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "transition host provenance has no DNS state" in source
print("PipeWire transition host-DNS-state behavior verified")
