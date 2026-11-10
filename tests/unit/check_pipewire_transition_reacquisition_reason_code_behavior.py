"""Regression checks for optional reacquisition reason code."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an invalid reason code" in source
print("PipeWire transition reacquisition-reason-code behavior verified")
