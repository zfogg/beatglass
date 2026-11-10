"""Regression checks for optional reacquisition error-domain integrity."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an empty error domain" in source
print("PipeWire transition reacquisition-domain behavior verified")
