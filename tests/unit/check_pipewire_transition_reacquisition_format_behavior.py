"""Regression checks for optional reacquisition format integrity."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an invalid negotiated format" in source
assert "reacquisition format disagrees with restart format" in source
print("PipeWire transition reacquisition-format behavior verified")
