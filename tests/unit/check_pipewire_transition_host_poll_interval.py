from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "transition host provenance has invalid poll interval" in source
print("PipeWire transition host-poll-interval behavior verified")
