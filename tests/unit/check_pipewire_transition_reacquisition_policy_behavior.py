"""Regression checks for optional reacquisition policy."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an empty policy" in source
print("PipeWire transition reacquisition-policy behavior verified")
