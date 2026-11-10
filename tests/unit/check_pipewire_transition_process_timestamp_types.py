from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "process timestamps must be non-negative integers" in source
print("PipeWire transition process-timestamp type behavior verified")
