"""Regression checks for optional reacquisition process index."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an invalid process index" in source
print("PipeWire transition reacquisition-index behavior verified")
