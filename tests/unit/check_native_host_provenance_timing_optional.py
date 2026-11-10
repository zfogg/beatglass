import json
import subprocess
import sys
import tempfile
from pathlib import Path

producer = Path("tests/pipewire/validate_native_host_provenance.py")
with tempfile.TemporaryDirectory() as directory:
    host = Path(directory) / "host.json"
    report = Path(directory) / "report.json"
    host.write_text(json.dumps({"platform": "linux", "name": "minimal-host"}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(producer), str(host), str(report)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    assert json.loads(report.read_text(encoding="utf-8"))["checked_fields"] == ["name", "platform"]
print("Native host provenance timing optionality verified")
