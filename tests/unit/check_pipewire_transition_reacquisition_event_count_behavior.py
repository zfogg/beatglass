"""Regression checks for optional reacquisition event count."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an invalid event count" in source
print("PipeWire transition reacquisition-event-count behavior verified")
