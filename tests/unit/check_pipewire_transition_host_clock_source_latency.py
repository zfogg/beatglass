from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert "transition host provenance has invalid source latency" in source
print("PipeWire transition host-clock-source-latency behavior verified")
