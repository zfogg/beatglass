"""Verify a completed Linux PipeWire qualification runner summary."""
import hashlib
import json
import sys
import subprocess
from pathlib import Path

if len(sys.argv) != 2:
    raise SystemExit("usage: verify_native_run_summary.py SUMMARY.json")
summary_path = Path(sys.argv[1])
summary = json.loads(summary_path.read_text(encoding="utf-8"))
if not summary.get("qualified"):
    raise SystemExit("native qualification summary is not qualified")
if summary.get("qualification_target") != "Linux/PipeWire" or summary.get("handoff_status") != "complete":
    raise SystemExit("native qualification summary has incomplete handoff status")
if not isinstance(summary.get("commit"), str) or not summary["commit"].strip():
    raise SystemExit("native qualification summary has no commit identity")
if summary.get("failed_command") is not None:
    raise SystemExit("native qualification has a failed command")
for key in ("native_discovery_json", "native_junit"):
    path = Path(summary[key])
    if not path.is_file():
        raise SystemExit(f"missing evidence artifact: {path}")
if summary.get("native_junit_sha256") != hashlib.sha256(Path(summary["native_junit"]).read_bytes()).hexdigest():
    raise SystemExit("native JUnit digest mismatch")
if summary.get("native_discovery_sha256") != hashlib.sha256(Path(summary["native_discovery_json"]).read_bytes()).hexdigest():
    raise SystemExit("native discovery digest mismatch")
if summary.get("native_transition_json"):
    transition = Path(summary["native_transition_json"])
    if not transition.is_file():
        raise SystemExit(f"missing transition evidence artifact: {transition}")
    if summary.get("native_transition_sha256") != hashlib.sha256(transition.read_bytes()).hexdigest():
        raise SystemExit("native transition digest mismatch")
    validator = Path(__file__).with_name("validate_pipewire_transition_evidence.py")
    result = subprocess.run([sys.executable, str(validator), str(transition)], capture_output=True, text=True, check=False)
    if result.returncode:
        raise SystemExit(result.stderr or result.stdout or "native transition evidence failed validation")
print("Native PipeWire qualification summary verified")
