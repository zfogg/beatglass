import subprocess
import tempfile
from pathlib import Path

with tempfile.TemporaryDirectory() as directory:
    missing = Path(directory) / "missing.json"
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(missing)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance is unreadable" in result.stderr

    malformed = Path(directory) / "malformed.json"
    malformed.write_text("{not-json", encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(malformed)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance is unreadable" in result.stderr

    duplicate = Path(directory) / "duplicate.json"
    duplicate.write_text('{"platform":"linux","name":"one","name":"two"}', encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(duplicate)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance is ambiguous" in result.stderr

    host = Path(directory) / "host.json"
    host.write_text('{"platform":"linux","name":"qualification-host"}', encoding="utf-8")
    result = subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(host), directory], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host provenance report is unwritable" in result.stderr
print("Native host provenance input rejection verified")
