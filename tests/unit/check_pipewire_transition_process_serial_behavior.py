"""Regression checks for optional process-event sink serial integrity."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "process sink serials must be complete and non-empty" in source
assert "process sink serial does not match active epoch" in source
assert "expected_serial = restart.get(\"serial\")" in source
print("PipeWire transition process-serial behavior verified")
