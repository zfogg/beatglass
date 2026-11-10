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
        "platform": "windows",
        "name": "qualified-test-host",
        "clock_accuracy_ns": 500,
        "clock_source_count": 3,
        "clock_source_label": "qpc",
    }), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(host), str(report)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    evidence = json.loads(report.read_text(encoding="utf-8"))
    assert evidence["checked_fields"] == ["clock_accuracy_ns", "clock_source_count", "clock_source_label", "name", "platform"]
print("Native host provenance timing unknown-field behavior verified")
