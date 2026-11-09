"""Exercise deterministic native evidence archive creation."""
import hashlib
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

packager = Path(__file__).parents[1] / "pipewire" / "package_native_evidence.py"
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    discovery = root / "pipewire-tests-discovery.json"
    junit = root / "pipewire-tests.xml"
    discovery.write_text('{"tests": [{"name": "capture"}]}\n', encoding="utf-8")
    junit.write_text("<testsuite tests='1'/>\n", encoding="utf-8")
    transitions = root / "pipewire-transitions.json"
    transitions.write_text(json.dumps({"schema":"beatglass-pipewire-transitions-v1", "platform":"Linux-test", "commit":"fixture-commit", "events":[
        {"event":"start", "epoch":1, "serial":"sink-a"},
        {"event":"process", "epoch":1, "sample_count":256},
        {"event":"error", "epoch":1, "error_code":-32, "discontinuity":True},
        {"event":"restart", "epoch":2, "serial":"sink-b", "format":{"sample_rate":48000, "channels":2}}
    ]}) + "\n", encoding="utf-8")
    summary = root / "native-summary.json"
    summary.write_text(json.dumps({"qualified": True, "commit": "fixture-commit", "failed_command": None, "native_discovery_json": str(discovery), "native_junit": str(junit), "native_transition_json": str(transitions), "native_transition_sha256": hashlib.sha256(transitions.read_bytes()).hexdigest(), "native_transition_event_count": 4, "native_discovery_sha256": hashlib.sha256(discovery.read_bytes()).hexdigest(), "native_junit_sha256": hashlib.sha256(junit.read_bytes()).hexdigest(), "commands": []}), encoding="utf-8")
    env = dict(os.environ, SOURCE_DATE_EPOCH="1700000000")
    for name in ("first.zip", "second.zip"):
        result = subprocess.run([sys.executable, str(packager), str(summary), str(root / name)], env=env, capture_output=True, text=True)
        if result.returncode != 0: raise SystemExit(result.stderr or result.stdout)
    if (root / "first.zip").read_bytes() != (root / "second.zip").read_bytes(): raise SystemExit("fixed SOURCE_DATE_EPOCH did not produce reproducible ZIP bytes")
    before = hashlib.sha256((root / "first.zip").read_bytes()).digest()
    result = subprocess.run([sys.executable, str(packager), str(summary), str(root / "first.zip")], env=env, capture_output=True, text=True)
    if result.returncode == 0 or "refusing to overwrite" not in result.stderr + result.stdout: raise SystemExit("existing output was overwritten")
    if hashlib.sha256((root / "first.zip").read_bytes()).digest() != before: raise SystemExit("existing output changed after refusal")
    left = root / "left"; right = root / "right"; left.mkdir(); right.mkdir()
    left_log = left / "command.log"; right_log = right / "command.log"
    left_log.write_text("left", encoding="utf-8"); right_log.write_text("right", encoding="utf-8")
    collision = dict(json.loads(summary.read_text(encoding="utf-8")), commands=[{"stdout": str(left_log), "stderr": str(right_log)}])
    collision_summary = root / "collision-summary.json"
    collision_summary.write_text(json.dumps(collision), encoding="utf-8")
    result = subprocess.run([sys.executable, str(packager), str(collision_summary), str(root / "collision.zip")], env=env, capture_output=True, text=True)
    if result.returncode == 0 or "colliding archive names" not in result.stderr + result.stdout: raise SystemExit("colliding evidence names were accepted")
    bad_transition = json.loads(summary.read_text(encoding="utf-8"))
    transitions.write_text(transitions.read_text(encoding="utf-8").replace("fixture-commit", "other-commit"), encoding="utf-8")
    bad_summary = root / "bad-transition-summary.json"
    bad_summary.write_text(json.dumps(bad_transition), encoding="utf-8")
    result = subprocess.run([sys.executable, str(packager), str(bad_summary), str(root / "bad-transition.zip")], env=env, capture_output=True, text=True)
    if result.returncode == 0 or "native_transition_json digest mismatch" not in result.stderr + result.stdout: raise SystemExit("invalid transition provenance was accepted")
print("PipeWire evidence packager behavior passed (4 scenarios)")
