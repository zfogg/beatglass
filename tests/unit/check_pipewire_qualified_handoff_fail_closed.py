"""Ensure a synthetic qualified handoff cannot bypass release provenance checks."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

validator = Path(__file__).parents[1] / "pipewire" / "validate_qualification_bundle.py"
matrix = {"schema": "beatglass-native-qualification-matrix-v1", "host_os": "Linux", "platforms": {
    "Linux": {"status": "qualified", "evidence": ["native.zip"]},
    "Windows": {"status": "pending", "evidence": [], "pending_reason": "host required"},
    "Darwin": {"status": "pending", "evidence": [], "pending_reason": "host required"},
}}
report = "# Native qualification matrix\nHost: `Linux`\n| Linux | **qualified** | yes | `native.zip` | — | — |\n| Windows | **pending** | no | — | host required | host |\n| Darwin | **pending** | no | — | host required | host |\n"
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    matrix_path, report_path = root / "matrix.json", root / "report.md"
    matrix_path.write_text(json.dumps(matrix), encoding="utf-8")
    report_path.write_text(report, encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(matrix_path), str(report_path)], capture_output=True, text=True)
    if result.returncode == 0 or "unattempted platform" not in result.stderr + result.stdout:
        raise SystemExit("qualified handoff without provenance was accepted")
print("Qualified handoff fail-closed check passed (1 scenario)")
