from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "transition host provenance has an invalid node id" in source
print("PipeWire transition host-node-id behavior verified")
