import json
import subprocess
import sys
import tempfile
from pathlib import Path

validator = Path("tests/pipewire/validate_native_host_provenance.py")
cases = [
    ({"clock_accuracy_ns": -1}, "invalid clock accuracy"),
    ({"clock_accuracy_ns": 1.5}, "invalid clock accuracy"),
    ({"clock_accuracy_ns": True}, "invalid clock accuracy"),
    ({"clock_source_count": 0}, "invalid clock source count"),
    ({"clock_source_count": 1.5}, "invalid clock source count"),
]
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / "host.json"
    for extra, expected in cases:
        payload = {"platform": "linux", "name": "test-host", **extra}
        path.write_text(json.dumps(payload), encoding="utf-8")
        result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
        assert result.returncode != 0
        assert expected in result.stderr
print("Native host provenance timing rejection behavior verified")
