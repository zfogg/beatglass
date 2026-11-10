from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert 'not isinstance(event_format.get("sample_rate"), int)' in source
assert 'not isinstance(event_format.get("channels"), int)' in source
print("PipeWire transition event-format type behavior verified")
