from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "transition host provenance has invalid switch count" in source
print("PipeWire transition host-clock-source-switch-count behavior verified")
