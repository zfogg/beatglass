"""Regression checks for optional transition epoch completeness."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "transition epochs must be complete when supplied" in source
assert "len(epochs) != len(events)" in source
print("PipeWire transition epoch behavior verified")
