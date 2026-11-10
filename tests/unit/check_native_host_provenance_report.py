import json
import subprocess
import tempfile
from pathlib import Path

source = Path("tests/pipewire/validate_native_host_provenance.py").read_text()
assert "usage: validate_native_host_provenance.py HOST.json [REPORT.json]" in source
assert '"checked_fields"' in source
print("Native host provenance report workflow verified")
with tempfile.TemporaryDirectory() as directory:
    host = Path(directory) / "host.json"
    report = Path(directory) / "report.json"
    host.write_text(json.dumps({"platform": "linux", "name": "qualification-host"}))
    subprocess.run(["python", "tests/pipewire/validate_native_host_provenance.py", str(host), str(report)], check=True)
    result = json.loads(report.read_text())
    assert result["schema"] == "beatglass-native-host-provenance-v1"
    assert result["valid"] is True
    assert result["platform"] == "linux"
