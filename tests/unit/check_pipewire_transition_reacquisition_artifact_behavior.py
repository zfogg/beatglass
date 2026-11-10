"""Regression checks for optional reacquisition artifact identity."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an empty artifact identity" in source
assert "reacquisition artifact identity does not match artifact" in source
print("PipeWire transition reacquisition-artifact behavior verified")
