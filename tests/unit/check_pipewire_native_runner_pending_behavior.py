"""Exercise honest pending semantics on a non-Linux host."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

root = Path(__file__).parents[1]
runner = root / "pipewire" / "run_linux_native_qualification.py"
plan_tool = root / "pipewire" / "prepare_native_host_capture.py"
with tempfile.TemporaryDirectory() as directory:
    output = Path(directory)
    plan = output / "plan.json"; summary = output / "summary.json"
    subprocess.run([sys.executable, str(plan_tool), str(plan)], check=True)
    result = subprocess.run([sys.executable, str(runner), "--source", str(root.parent), "--summary", str(summary), "--capture-plan", str(plan)], capture_output=True, text=True)
    if result.returncode == 0: raise SystemExit("non-Linux native runner claimed qualification")
    payload = json.loads(summary.read_text(encoding="utf-8"))
    if payload.get("handoff_status") != "pending" or payload.get("qualified") is not False or payload.get("capture_plan_validated") is not True or payload.get("native_execution_attempted") is not False:
        raise SystemExit("pending native handoff fields were not recorded honestly")
    invalid_plan = output / "invalid-plan.json"
    invalid_plan.write_text("{}", encoding="utf-8")
    rejected = subprocess.run([sys.executable, str(runner), "--source", str(root.parent), "--capture-plan", str(invalid_plan)], capture_output=True, text=True)
    if rejected.returncode == 0 or "native host capture plan" not in rejected.stderr + rejected.stdout:
        raise SystemExit("invalid capture plan was accepted")
print("PipeWire native runner pending behavior passed (2 scenarios)")
