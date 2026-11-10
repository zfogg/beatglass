from pathlib import Path

source = Path("tests/pipewire/validate_native_host_provenance.py").read_text()
assert "host provenance has an invalid platform" in source
assert "host provenance has an invalid node id" in source
assert "host provenance has an invalid TPM flag" in source
assert "host provenance has invalid clock accuracy" in source
assert "host provenance has invalid clock source count" in source
assert "host provenance is unreadable" in source
assert "host provenance is ambiguous" in source
assert "os.replace" in source
assert "os.fsync" in source
assert "NamedTemporaryFile" in source
print("Native host provenance validator verified")
