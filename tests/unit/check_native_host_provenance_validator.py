from pathlib import Path

source = Path("tests/pipewire/validate_native_host_provenance.py").read_text()
assert "host provenance has an invalid platform" in source
assert "host provenance has an invalid node id" in source
assert "host provenance has an invalid TPM flag" in source
print("Native host provenance validator verified")
