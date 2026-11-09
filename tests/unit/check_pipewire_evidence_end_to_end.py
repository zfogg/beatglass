"""Exercise packaging and independent verification as one evidence handoff."""
import hashlib
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).parents[1]
packager = ROOT / "pipewire" / "package_native_evidence.py"
verifier = ROOT / "pipewire" / "verify_native_evidence_archive.py"

with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    discovery = root / "pipewire-tests-discovery.json"
    junit = root / "pipewire-tests.xml"
    transitions = root / "pipewire-transitions.json"
    discovery.write_text('{"tests":[{"name":"capture"}]}\n', encoding="utf-8")
    junit.write_text("<testsuite tests='1'><testcase name='capture'/></testsuite>\n", encoding="utf-8")
    transitions.write_text(json.dumps({"schema":"beatglass-pipewire-transitions-v1", "platform":"Linux-test", "commit":"fixture-commit", "events":[
        {"event":"start","epoch":1,"serial":"sink-a"},
        {"event":"process","epoch":1,"sample_count":256},
        {"event":"error","epoch":1,"error_code":-32,"discontinuity":True},
        {"event":"restart","epoch":2,"serial":"sink-b","format":{"sample_rate":48000,"channels":2}}
    ]}) + "\n", encoding="utf-8")
    summary = root / "native-summary.json"
    summary.write_text(json.dumps({"qualified":True,"commit":"fixture-commit","platform":"Linux-test","failed_command":None,
        "native_test_count":1,"native_test_name_match":True,"native_junit_test_count":1,"native_junit_failures":0,
        "native_junit_errors":0,"native_junit_skipped":0,"native_junit_exists":True,
        "native_test_discovery":{"count":1,"tests":[{"name":"capture"}]},"native_discovery_json":str(discovery),
        "native_junit":str(junit),"native_transition_json":str(transitions),"native_transition_event_count":4,
        "native_discovery_sha256":hashlib.sha256(discovery.read_bytes()).hexdigest(),
        "native_junit_sha256":hashlib.sha256(junit.read_bytes()).hexdigest(),
        "native_transition_sha256":hashlib.sha256(transitions.read_bytes()).hexdigest(),"commands":[]}), encoding="utf-8")
    archive = root / "evidence.zip"
    env = dict(os.environ, SOURCE_DATE_EPOCH="1700000000")
    packaged = subprocess.run([sys.executable, str(packager), str(summary), str(archive)], env=env, capture_output=True, text=True)
    if packaged.returncode != 0:
        raise SystemExit(packaged.stderr or packaged.stdout)
    verified = subprocess.run([sys.executable, str(verifier), str(archive)], capture_output=True, text=True)
    if verified.returncode != 0:
        raise SystemExit(verified.stderr or verified.stdout)
print("PipeWire evidence end-to-end handoff passed (1 scenario)")
