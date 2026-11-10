import json
import subprocess
import sys
import tempfile
from pathlib import Path

validator = Path("tests/pipewire/validate_native_host_provenance.py")
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / "host.json"
    path.write_text(json.dumps({
        "platform": "linux",
        "name": "qualified-test-host",
        "clock_accuracy_ns": 0,
        "clock_source_count": 1,
    }), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    assert "native host provenance verified" in result.stdout
print("Native host provenance timing acceptance behavior verified")
