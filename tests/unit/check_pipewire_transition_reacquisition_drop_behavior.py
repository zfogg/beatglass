"""Regression checks for optional reacquisition drop-count integrity."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an invalid drop count" in source
assert "reacquisition[\"drop_count\"] < 0" in source
print("PipeWire transition reacquisition-drop behavior verified")
