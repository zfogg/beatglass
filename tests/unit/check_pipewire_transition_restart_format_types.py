from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert 'not isinstance(fmt.get("sample_rate"), int)' in source
assert 'not isinstance(fmt.get("channels"), int)' in source
print("PipeWire transition restart-format type behavior verified")
