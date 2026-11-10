import json
import subprocess
import sys
import tempfile
from pathlib import Path

producer = Path("tests/pipewire/validate_native_host_provenance.py")
with tempfile.TemporaryDirectory() as directory:
    host = Path(directory) / "host.json"
    host.write_text(json.dumps({"platform": "linux", "name": -1, "clock_accuracy_ns": 1}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(producer), str(host)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "no name" in result.stderr
print("Native host provenance negative name rejection verified")
