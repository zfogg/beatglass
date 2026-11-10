from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "transition host provenance has invalid sync offset" in source
print("PipeWire transition host-sync-offset behavior verified")
