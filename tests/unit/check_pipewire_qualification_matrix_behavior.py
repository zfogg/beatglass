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
    if set(matrix["platforms"]) != {"Linux", "Windows", "Darwin"}:
        raise SystemExit("qualification matrix omitted a required platform")
    if any("pending_reason" not in item for name, item in matrix["platforms"].items() if name != matrix["host_os"]):
        raise SystemExit("pending platform lacks an actionable reason")
    if host_entry["status"] != "pending" or host_entry["evidence"] != ["evidence/native-summary.json"] or host_entry["pending_reason"] != "host unavailable":
        raise SystemExit("runner summary was not merged into host matrix entry")
    summary.write_text(json.dumps({"qualified": True, "native_artifacts": ["evidence/native-qualification.zip"]}), encoding="utf-8")
    qualified = root / "qualified-matrix.json"
    result = subprocess.run([sys.executable, str(script), str(qualified), "--summary", str(summary)], capture_output=True, text=True)
    if result.returncode != 0:
        raise SystemExit(result.stderr or result.stdout)
    qualified_entry = json.loads(qualified.read_text(encoding="utf-8"))["platforms"][matrix["host_os"]]
    if qualified_entry["status"] != "qualified" or qualified_entry["evidence"] != ["evidence/native-qualification.zip"]:
        raise SystemExit("qualified runner summary was not merged into host matrix entry")
print("PipeWire qualification matrix behavior passed (2 scenarios)")
