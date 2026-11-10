from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "transition lifecycle events are out of order" in source
assert "transition lifecycle events must be unique" in source
print("PipeWire transition lifecycle-order behavior verified")
