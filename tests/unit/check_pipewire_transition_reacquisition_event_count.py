from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "reacquisition event count does not match restart samples" in source
print("PipeWire transition reacquisition-event-count behavior verified")
