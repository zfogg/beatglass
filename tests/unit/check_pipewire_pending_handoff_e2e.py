"""Exercise the pending native runner -> matrix -> report handoff."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

root = Path(__file__).parents[1]
runner = root / "pipewire" / "run_linux_native_qualification.py"
plan_tool = root / "pipewire" / "prepare_native_host_capture.py"
matrix_tool = root / "pipewire" / "qualification_matrix.py"
validator = root / "pipewire" / "validate_qualification_matrix.py"
renderer = root / "pipewire" / "render_qualification_matrix_report.py"

with tempfile.TemporaryDirectory() as directory:
    out = Path(directory)
    plan, summary = out / "plan.json", out / "summary.json"
    matrix, report = out / "matrix.json", out / "matrix.md"
    subprocess.run([sys.executable, str(plan_tool), str(plan)], check=True)
    pending = subprocess.run([sys.executable, str(runner), "--source", str(root.parent), "--summary", str(summary), "--capture-plan", str(plan)], capture_output=True, text=True)
    if pending.returncode == 0:
        raise SystemExit("pending native runner claimed success")
    summary_data = json.loads(summary.read_text(encoding="utf-8"))
    if summary_data.get("native_execution_attempted") is not False:
        raise SystemExit("pending runner did not record a false execution attempt")
    subprocess.run([sys.executable, str(matrix_tool), str(matrix), "--summary", str(summary)], check=True)
    subprocess.run([sys.executable, str(validator), str(matrix)], check=True)
    subprocess.run([sys.executable, str(renderer), str(matrix), str(report)], check=True)
    rendered = report.read_text(encoding="utf-8")
    for marker in ("Native attempt", "no", "Linux host", "PipeWire daemon"):
        if marker not in rendered:
            raise SystemExit(f"pending handoff report missing {marker!r}")
print("PipeWire pending handoff e2e passed (1 scenario)")
