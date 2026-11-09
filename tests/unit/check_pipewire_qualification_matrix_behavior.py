"""Exercise matrix generation with runner summary evidence."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

script = Path(__file__).parents[1] / "pipewire" / "qualification_matrix.py"
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    summary = root / "summary.json"
    output = root / "matrix.json"
    summary.write_text(json.dumps({"qualified": False, "pending_reason": "host unavailable", "native_artifacts": ["evidence/native-summary.json"]}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(script), str(output), "--summary", str(summary)], capture_output=True, text=True)
    if result.returncode != 0:
        raise SystemExit(result.stderr or result.stdout)
    matrix = json.loads(output.read_text(encoding="utf-8"))
    host_entry = matrix["platforms"][matrix["host_os"]]
    if host_entry["status"] != "pending" or host_entry["evidence"] != ["evidence/native-summary.json"] or host_entry["pending_reason"] != "host unavailable":
        raise SystemExit("runner summary was not merged into host matrix entry")
print("PipeWire qualification matrix behavior passed (1 scenario)")
