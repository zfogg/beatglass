"""Regression checks for optional reacquisition end timestamp."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an invalid end timestamp" in source
assert "reacquisition end precedes start" in source
print("PipeWire transition reacquisition-end behavior verified")
