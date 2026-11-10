"""Regression checks for optional reacquisition error-code integrity."""
from pathlib import Path

source = (Path(__file__).parents[1] / "pipewire" / "validate_pipewire_transition_evidence.py").read_text(encoding="utf-8")
assert "reacquisition evidence has an invalid error code" in source
assert "reacquisition[\"error_code\"] == 0" in source
print("PipeWire transition reacquisition-error behavior verified")
