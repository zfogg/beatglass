"""Verify a completed Linux PipeWire qualification runner summary."""
import hashlib
import json
import sys
import subprocess
from datetime import datetime
from pathlib import Path

if len(sys.argv) != 2:
    raise SystemExit("usage: verify_native_run_summary.py SUMMARY.json")
summary_path = Path(sys.argv[1])
summary = json.loads(summary_path.read_text(encoding="utf-8"))
if summary.get("runner_summary_schema") != "beatglass-native-runner-summary-v1":
    raise SystemExit("native qualification summary has unsupported runner schema")
try:
    captured_at = datetime.fromisoformat(str(summary.get("captured_at", "")).replace("Z", "+00:00"))
    if captured_at.tzinfo is None:
        raise ValueError
except ValueError:
    raise SystemExit("native qualification summary has invalid capture timestamp")
if not isinstance(summary.get("environment"), dict) or not summary["environment"].get("python"):
    raise SystemExit("native qualification summary has incomplete environment identity")
if not summary.get("host_os"):
    raise SystemExit("native qualification summary has no host OS identity")
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
    if not str(summary.get("platform", "")).lower().startswith("linux"):
        raise SystemExit("native transition evidence is not from a Linux host")
    transition = Path(summary["native_transition_json"])
    if not transition.is_file():
        raise SystemExit(f"missing transition evidence artifact: {transition}")
    if summary.get("native_transition_sha256") != hashlib.sha256(transition.read_bytes()).hexdigest():
        raise SystemExit("native transition digest mismatch")
    validator = Path(__file__).with_name("validate_pipewire_transition_evidence.py")
    result = subprocess.run([sys.executable, str(validator), str(transition)], capture_output=True, text=True, check=False)
    if result.returncode:
        raise SystemExit(result.stderr or result.stdout or "native transition evidence failed validation")
    transition_payload = json.loads(transition.read_text(encoding="utf-8"))
    transition_events = transition_payload.get("events", transition_payload)
    if transition_payload.get("platform") != summary.get("platform"):
        raise SystemExit("native transition platform mismatch")
    if transition_payload.get("commit") != summary.get("commit"):
        raise SystemExit("native transition commit mismatch")
    count = summary.get("native_transition_event_count")
    if not isinstance(count, int) or isinstance(count, bool) or count <= 0 or count != len(transition_events):
        raise SystemExit("native transition event count mismatch")
print("Native PipeWire qualification summary verified")
