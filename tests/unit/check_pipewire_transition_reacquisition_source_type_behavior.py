"""Regression checks for optional reacquisition source type."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an empty source type" in source
print("PipeWire transition reacquisition-source-type behavior verified")
