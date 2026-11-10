import subprocess
import tempfile
from pathlib import Path

with tempfile.TemporaryDirectory() as directory:
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
print("Native host provenance input rejection verified")
