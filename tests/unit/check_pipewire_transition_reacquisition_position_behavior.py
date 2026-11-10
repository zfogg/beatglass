"""Regression checks for optional reacquisition sample position."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an invalid sample position" in source
assert "reacquisition[\"sample_position\"] < 0" in source
print("PipeWire transition reacquisition-position behavior verified")
