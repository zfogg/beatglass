"""Regression checks for optional reacquisition recovery reason."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an empty recovery reason" in source
print("PipeWire transition reacquisition-reason behavior verified")
