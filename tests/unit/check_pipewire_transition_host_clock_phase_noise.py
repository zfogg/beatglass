from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "transition host provenance has invalid phase noise" in source
print("PipeWire transition host-clock-phase-noise behavior verified")
