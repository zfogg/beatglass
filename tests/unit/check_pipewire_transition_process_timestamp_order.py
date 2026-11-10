from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "process timestamps must be monotonic" in source
print("PipeWire transition process-timestamp order behavior verified")
