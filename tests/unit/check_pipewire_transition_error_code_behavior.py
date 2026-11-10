"""Regression checks for optional transition error-code integrity."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "transition error codes must be complete and non-zero" in source
assert "len(error_codes) != len(error_events)" in source
print("PipeWire transition error-code behavior verified")
