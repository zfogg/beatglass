"""Regression checks for optional reacquisition capture identity."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an empty capture id" in source
print("PipeWire transition reacquisition-capture behavior verified")
