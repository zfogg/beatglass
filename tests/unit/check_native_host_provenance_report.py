import json
import subprocess
import tempfile
from pathlib import Path

source = Path("tests/pipewire/validate_native_host_provenance.py").read_text()
assert "usage: validate_native_host_provenance.py HOST.json [REPORT.json]" in source
assert '"checked_fields"' in source
assert "os.fsync" in Path("tests/pipewire/validate_native_host_provenance.py").read_text()
assert "os.name != \"nt\"" in Path("tests/pipewire/validate_native_host_provenance.py").read_text()
assert "temporary_path = None" in Path("tests/pipewire/validate_native_host_provenance.py").read_text()
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
    result = json.loads(report.read_text())
    assert result["schema"] == "beatglass-native-host-provenance-v1"
    assert result["valid"] is True
    assert result["platform"] == "linux"
    subprocess.run(["python", "tests/pipewire/validate_native_host_provenance_report.py", str(report)], check=True)
