"""Ensure a fully attributed qualified handoff can pass release consumption."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

validator = Path(__file__).parents[1] / "pipewire" / "validate_qualification_bundle.py"
matrix = {"schema": "beatglass-native-qualification-matrix-v1", "host_os": "Linux", "commit": "native-commit", "platforms": {
    "Linux": {"status": "qualified", "evidence": ["native.zip"], "native_execution_attempted": True, "execution_blockers": []},
    "Windows": {"status": "pending", "evidence": [], "pending_reason": "host required", "native_execution_attempted": False, "execution_blockers": ["Windows host"]},
    "Darwin": {"status": "pending", "evidence": [], "pending_reason": "host required", "native_execution_attempted": False, "execution_blockers": ["macOS host"]},
}}
report = "# Native qualification matrix\nHost: `Linux`\nCommit: `native-commit`\n| Platform | Status | Native attempt | Evidence | Pending reason | Blockers |\n| --- | --- | --- | --- | --- | --- |\n| Linux | **qualified** | yes | `native.zip` | — | — |\n| Windows | **pending** | no | — | host required | Windows host |\n| Darwin | **pending** | no | — | host required | macOS host |\n"
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    matrix_path, report_path = root / "matrix.json", root / "report.md"
    matrix_path.write_text(json.dumps(matrix), encoding="utf-8")
    report_path.write_text(report, encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(matrix_path), str(report_path)], capture_output=True, text=True)
    if result.returncode != 0:
        raise SystemExit(result.stderr or result.stdout)
print("Qualified handoff provenance acceptance passed (1 scenario)")
