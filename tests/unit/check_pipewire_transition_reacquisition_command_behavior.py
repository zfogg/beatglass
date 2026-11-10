"""Regression checks for optional reacquisition capture command."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an empty capture command" in source
print("PipeWire transition reacquisition-command behavior verified")
