import json
import subprocess
import sys
import tempfile
from pathlib import Path

producer = Path("tests/pipewire/validate_native_host_provenance.py")
consumer = Path("tests/pipewire/validate_native_host_provenance_report.py")
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    for platform in ("linux", "windows", "macos"):
        host = root / f"{platform}.json"
        report = root / f"{platform}-report.json"
        host.write_text(json.dumps({"platform": platform, "name": f"{platform}-host", "clock_accuracy_ns": 100, "clock_source_count": 1}), encoding="utf-8")
        produced = subprocess.run([sys.executable, str(producer), str(host), str(report)], capture_output=True, text=True)
        assert produced.returncode == 0, produced.stderr
        consumed = subprocess.run([sys.executable, str(consumer), str(report)], capture_output=True, text=True)
        assert consumed.returncode == 0, consumed.stderr
        assert json.loads(report.read_text(encoding="utf-8"))["platform"] == platform
print("Native host provenance timing platform matrix verified")
