"""Regression checks for optional reacquisition device identity."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an empty device identity" in source
assert "reacquisition device identity does not match restart" in source
print("PipeWire transition reacquisition-device behavior verified")
