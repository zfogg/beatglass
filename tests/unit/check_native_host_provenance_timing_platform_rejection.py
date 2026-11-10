import json
import subprocess
import sys
import tempfile
from pathlib import Path

producer = Path("tests/pipewire/validate_native_host_provenance.py")
with tempfile.TemporaryDirectory() as directory:
    host = Path(directory) / "host.json"
    host.write_text(json.dumps({"platform": "solaris", "name": "test-host", "clock_accuracy_ns": 100, "clock_source_count": 1}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(producer), str(host)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "invalid platform" in result.stderr
print("Native host provenance timing platform rejection verified")
