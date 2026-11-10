"""Regression checks for optional reacquisition artifact digest."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an invalid artifact digest" in source
assert "reacquisition artifact digest does not match artifact" in source
print("PipeWire transition reacquisition-digest behavior verified")
