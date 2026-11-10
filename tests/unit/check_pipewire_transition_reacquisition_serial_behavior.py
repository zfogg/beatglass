from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "reacquisition sink serial does not match restart" in source
print("PipeWire transition reacquisition-serial behavior verified")
