import json
import subprocess
import sys
import tempfile
from pathlib import Path

validator = Path("tests/pipewire/validate_native_host_provenance.py")
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    host = root / "host.json"
    report = root / "report.json"
    host.write_text(json.dumps({
        "platform": "linux",
        "name": "qualified-test-host",
        "clock_accuracy_ns": 250,
        "clock_source_count": 2,
    }), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(host), str(report)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    evidence = json.loads(report.read_text(encoding="utf-8"))
    assert evidence["valid"] is True
    assert evidence["checked_fields"] == ["clock_accuracy_ns", "clock_source_count", "name", "platform"]
print("Native host provenance timing report verified")
