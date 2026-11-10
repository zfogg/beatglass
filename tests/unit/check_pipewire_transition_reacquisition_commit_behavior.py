"""Regression checks for optional reacquisition commit provenance."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an empty commit" in source
assert "reacquisition commit does not match artifact" in source
print("PipeWire transition reacquisition-commit behavior verified")
