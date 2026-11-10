from pathlib import Path

source = Path("tests/pipewire/validate_native_host_provenance_report.py").read_text()
assert "host provenance report has an unsupported schema" in source
assert "host provenance report has invalid checked fields" in source
print("Native host provenance report validator verified")
