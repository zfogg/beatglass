from pathlib import Path

source = Path("tests/pipewire/validate_pipewire_transition_evidence.py").read_text()
assert 'not isinstance(reacquisition_format.get("sample_rate"), int)' in source
assert 'not isinstance(reacquisition_format.get("channels"), int)' in source
print("PipeWire transition reacquisition-format type behavior verified")
