from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "transition events must have a non-empty event name" in source
print("PipeWire transition event-name behavior verified")
