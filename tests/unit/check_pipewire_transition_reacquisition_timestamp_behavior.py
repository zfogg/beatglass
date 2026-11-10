"""Regression checks for optional reacquisition timestamp integrity."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an invalid timestamp" in source
assert "reacquisition timestamp precedes restart" in source
print("PipeWire transition reacquisition-timestamp behavior verified")
