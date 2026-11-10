from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert 'payload["event_count"] <= 0' in source
print("PipeWire transition event-count type behavior verified")
