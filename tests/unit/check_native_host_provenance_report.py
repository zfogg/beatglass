import json
import os
import stat
import subprocess
import tempfile
from pathlib import Path

source = Path("tests/pipewire/validate_native_host_provenance.py").read_text()
assert "usage: validate_native_host_provenance.py HOST.json [REPORT.json]" in source
assert '"checked_fields"' in source
assert "os.fsync" in Path("tests/pipewire/validate_native_host_provenance.py").read_text()
assert "os.name != \"nt\"" in Path("tests/pipewire/validate_native_host_provenance.py").read_text()
assert "temporary_path = None" in Path("tests/pipewire/validate_native_host_provenance.py").read_text()
assert "existing_mode" in Path("tests/pipewire/validate_native_host_provenance.py").read_text()
print("Native host provenance report workflow verified")
with tempfile.TemporaryDirectory() as directory:
    host = Path(directory) / "host.json"
    report = Path(directory) / "report.json"
    host.write_text(json.dumps({"platform": "linux", "name": "qualification-host"}))
    subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(host), str(report)], check=True)
    assert report.exists()
    assert not list(report.parent.glob("tmp*"))
    nested = Path(directory) / "nested" / "evidence" / "report.json"
    subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(host), str(nested)], check=True)
    assert nested.exists()
    if os.name != "nt":
        os.chmod(report, 0o640)
        subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(host), str(report)], check=True)
        assert stat.S_IMODE(report.stat().st_mode) == 0o640
    optional_host = Path(directory) / "optional-host.json"
    optional_report = Path(directory) / "optional-report.json"
    optional_host.write_text(json.dumps({"platform": "linux", "name": "qualified", "node_id": 7, "secure_boot": True, "tpm_present": False}), encoding="utf-8")
    subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(optional_host), str(optional_report)], check=True)
    optional_result = json.loads(optional_report.read_text(encoding="utf-8"))
    assert optional_result["checked_fields"] == ["name", "node_id", "platform", "secure_boot", "tpm_present"]
    subprocess.run(["python", "tests/pipewire/validate_native_host_provenance_report.py", str(optional_report)], check=True)
    forward_host = Path(directory) / "forward-host.json"
    forward_report = Path(directory) / "forward-report.json"
    forward_host.write_text(json.dumps({"platform": "linux", "name": "qualified", "future_measurement": "retained"}), encoding="utf-8")
    subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(forward_host), str(forward_report)], check=True)
    assert "future_measurement" in json.loads(forward_report.read_text(encoding="utf-8"))["checked_fields"]
    if os.name != "nt":
        os.chmod(optional_report, 0o640)
        subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(optional_host), str(optional_report)], check=True)
        assert stat.S_IMODE(optional_report.stat().st_mode) == 0o640
    result = json.loads(report.read_text())
    assert result["schema"] == "beatglass-native-host-provenance-v1"
    assert result["valid"] is True
    assert result["platform"] == "linux"
    subprocess.run(["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)], check=True)
