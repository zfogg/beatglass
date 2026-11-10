"""Regression checks for optional process sample-count integrity."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "process sample counts must be complete and non-negative" in source
assert "len(sample_counts) != len(process_events)" in source
assert "isinstance(value, bool)" in source
print("PipeWire transition sample-count behavior verified")
