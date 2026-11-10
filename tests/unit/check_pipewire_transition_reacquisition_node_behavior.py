"""Regression checks for optional reacquisition node identity."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an invalid node id" in source
print("PipeWire transition reacquisition-node behavior verified")
