import json
import subprocess
import sys
import tempfile
from pathlib import Path

producer = Path("tests/pipewire/validate_native_host_provenance.py")
consumer = Path("tests/pipewire/validate_native_host_provenance_report.py")
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    host = root / "host.json"
    report = root / "report.json"
    name = "測定ホスト"
    host.write_text(json.dumps({"platform": "linux", "name": name}, ensure_ascii=False), encoding="utf-8")
    produced = subprocess.run([sys.executable, str(producer), str(host), str(report)], capture_output=True, text=True)
    assert produced.returncode == 0, produced.stderr
    consumed = subprocess.run([sys.executable, str(consumer), str(report)], capture_output=True, text=True)
    assert consumed.returncode == 0, consumed.stderr
    assert json.loads(report.read_text(encoding="utf-8"))["name"] == name
print("Native host provenance Unicode identity verified")
