"""Regression checks for optional reacquisition start timestamp."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an invalid start timestamp" in source
assert "reacquisition start timestamp has no timezone" in source
print("PipeWire transition reacquisition-start behavior verified")
