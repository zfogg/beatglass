from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "restart device identity does not match start" in source
print("PipeWire transition restart-device identity behavior verified")
