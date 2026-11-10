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
    summary.write_text(json.dumps({"qualified": False, "pending_reason": "host unavailable", "native_artifacts": ["evidence/native-summary.json"], "native_execution_attempted": False, "native_execution_blockers": ["Linux host"]}), encoding="utf-8")
    result = subprocess.run([sys.executable, str(script), str(output), "--summary", str(summary)], capture_output=True, text=True)
    if result.returncode != 0:
        raise SystemExit(result.stderr or result.stdout)
    matrix = json.loads(output.read_text(encoding="utf-8"))
    host_entry = matrix["platforms"][matrix["host_os"]]
    if set(matrix["platforms"]) != {"Linux", "Windows", "Darwin"}:
        raise SystemExit("qualification matrix omitted a required platform")
    if any("pending_reason" not in item for name, item in matrix["platforms"].items() if name != matrix["host_os"]):
        raise SystemExit("pending platform lacks an actionable reason")
    if host_entry["status"] != "pending" or host_entry["evidence"] != ["evidence/native-summary.json"] or host_entry["pending_reason"] != "host unavailable" or host_entry["native_execution_attempted"] is not False or host_entry["execution_blockers"] != ["Linux host"]:
        raise SystemExit("runner summary was not merged into host matrix entry")
    summary.write_text(json.dumps({"qualified": True, "native_artifacts": ["evidence/native-qualification.zip"]}), encoding="utf-8")
    qualified = root / "qualified-matrix.json"
    result = subprocess.run([sys.executable, str(script), str(qualified), "--summary", str(summary)], capture_output=True, text=True)
    if result.returncode != 0:
        raise SystemExit(result.stderr or result.stdout)
    qualified_entry = json.loads(qualified.read_text(encoding="utf-8"))["platforms"][matrix["host_os"]]
    if qualified_entry["status"] != "qualified" or qualified_entry["evidence"] != ["evidence/native-qualification.zip"]:
        raise SystemExit("qualified runner summary was not merged into host matrix entry")
    absolute = root / "evidence" / "native-absolute.zip"
    absolute.parent.mkdir()
    absolute.write_bytes(b"evidence")
    summary.write_text(json.dumps({"qualified": True, "native_artifacts": [str(absolute)]}), encoding="utf-8")
    absolute_matrix = root / "absolute-matrix.json"
    result = subprocess.run([sys.executable, str(script), str(absolute_matrix), "--summary", str(summary)], capture_output=True, text=True)
    if result.returncode != 0:
        raise SystemExit(result.stderr or result.stdout)
    absolute_entry = json.loads(absolute_matrix.read_text(encoding="utf-8"))["platforms"][matrix["host_os"]]
    if absolute_entry["evidence"] != ["evidence/native-absolute.zip"]:
        raise SystemExit("absolute evidence path was not made portable")
    summary.write_text(json.dumps({"qualified": True, "native_artifacts": "evidence.zip"}), encoding="utf-8")
    malformed = root / "malformed-matrix.json"
    result = subprocess.run([sys.executable, str(script), str(malformed), "--summary", str(summary)], capture_output=True, text=True)
    if result.returncode == 0 or "invalid native_artifacts" not in result.stderr + result.stdout:
        raise SystemExit("malformed native_artifacts summary was accepted")
    summary.write_text(json.dumps({"qualified": False, "native_artifacts": [42], "pending_reason": "host unavailable"}), encoding="utf-8")
    invalid_artifact = root / "invalid-artifact-entry-matrix.json"
    result = subprocess.run([sys.executable, str(script), str(invalid_artifact), "--summary", str(summary)], capture_output=True, text=True)
    if result.returncode == 0 or "invalid native_artifacts" not in result.stderr + result.stdout:
        raise SystemExit("non-string native artifact entry was accepted")
    summary.write_text(json.dumps({"qualified": False, "native_artifacts": ["evidence.zip", "evidence.zip"], "pending_reason": "host unavailable"}), encoding="utf-8")
    duplicate_artifact = root / "duplicate-artifact-matrix.json"
    result = subprocess.run([sys.executable, str(script), str(duplicate_artifact), "--summary", str(summary)], capture_output=True, text=True)
    if result.returncode == 0 or "duplicate native_artifacts" not in result.stderr + result.stdout:
        raise SystemExit("duplicate native artifact entries were accepted")
    first_external = root / "one" / "evidence.zip"
    second_external = root / "two" / "evidence.zip"
    first_external.parent.mkdir()
    second_external.parent.mkdir()
    first_external.write_bytes(b"one")
    second_external.write_bytes(b"two")
    summary.write_text(json.dumps({"qualified": False, "native_artifacts": [str(first_external), str(second_external)], "pending_reason": "host unavailable"}), encoding="utf-8")
    collision = root / "collision-matrix.json"
    result = subprocess.run([sys.executable, str(script), str(collision), "--summary", str(summary)], capture_output=True, text=True)
    if result.returncode == 0 or "collide after normalization" not in result.stderr + result.stdout:
        raise SystemExit("normalized evidence collision was accepted")
    summary.write_text(json.dumps({"qualified": "true", "native_artifacts": []}), encoding="utf-8")
    invalid_status = root / "invalid-status-matrix.json"
    result = subprocess.run([sys.executable, str(script), str(invalid_status), "--summary", str(summary)], capture_output=True, text=True)
    if result.returncode == 0 or "invalid qualified status" not in result.stderr + result.stdout:
        raise SystemExit("malformed qualified summary status was accepted")
    summary.write_text(json.dumps({"qualified": False, "native_artifacts": [], "native_execution_attempted": "false"}), encoding="utf-8")
    invalid_attempt = root / "invalid-attempt-matrix.json"
    result = subprocess.run([sys.executable, str(script), str(invalid_attempt), "--summary", str(summary)], capture_output=True, text=True)
    if result.returncode == 0 or "invalid execution attempt" not in result.stderr + result.stdout:
        raise SystemExit("malformed native execution attempt was accepted")
    summary.write_text(json.dumps({"qualified": False, "native_artifacts": [], "native_execution_blockers": "Linux host"}), encoding="utf-8")
    invalid_blockers = root / "invalid-blockers-matrix.json"
    result = subprocess.run([sys.executable, str(script), str(invalid_blockers), "--summary", str(summary)], capture_output=True, text=True)
    if result.returncode == 0 or "invalid execution blockers" not in result.stderr + result.stdout:
        raise SystemExit("malformed native execution blockers were accepted")
    summary.write_text(json.dumps({"qualified": False, "native_artifacts": [], "pending_reason": ""}), encoding="utf-8")
    invalid_reason = root / "invalid-reason-matrix.json"
    result = subprocess.run([sys.executable, str(script), str(invalid_reason), "--summary", str(summary)], capture_output=True, text=True)
    if result.returncode == 0 or "invalid pending reason" not in result.stderr + result.stdout:
        raise SystemExit("malformed pending reason was accepted")
    summary.write_text(json.dumps({"qualified": True, "native_artifacts": ["evidence.zip"]}), encoding="utf-8")
    missing_commit = root / "missing-commit-matrix.json"
    result = subprocess.run([sys.executable, str(script), str(missing_commit), "--summary", str(summary)], capture_output=True, text=True)
    if result.returncode == 0 or "missing commit provenance" not in result.stderr + result.stdout:
        raise SystemExit("qualified summary without commit provenance was accepted")
    summary.write_text(json.dumps({"qualified": True, "commit": "native-commit", "native_artifacts": ["evidence.zip"], "native_execution_attempted": False}), encoding="utf-8")
    missing_attempt = root / "missing-attempt-matrix.json"
    result = subprocess.run([sys.executable, str(script), str(missing_attempt), "--summary", str(summary)], capture_output=True, text=True)
    if result.returncode == 0 or "lacks native execution attempt" not in result.stderr + result.stdout:
        raise SystemExit("qualified summary without native execution attempt was accepted")
    summary.write_text(json.dumps({"qualified": True, "commit": "native-commit", "native_artifacts": ["evidence.zip"], "native_execution_attempted": True, "native_execution_blockers": ["permission"]}), encoding="utf-8")
    retained_blockers = root / "retained-blockers-matrix.json"
    result = subprocess.run([sys.executable, str(script), str(retained_blockers), "--summary", str(summary)], capture_output=True, text=True)
    if result.returncode == 0 or "retains execution blockers" not in result.stderr + result.stdout:
        raise SystemExit("qualified summary with execution blockers was accepted")
    summary.write_text(json.dumps({"qualified": False, "native_artifacts": [], "pending_reason": "host unavailable", "commit": 42}), encoding="utf-8")
    invalid_commit = root / "invalid-commit-matrix.json"
    result = subprocess.run([sys.executable, str(script), str(invalid_commit), "--summary", str(summary)], capture_output=True, text=True)
    if result.returncode == 0 or "invalid commit provenance" not in result.stderr + result.stdout:
        raise SystemExit("malformed pending commit provenance was accepted")
    summary.write_text(json.dumps({"qualified": False, "native_artifacts": [], "pending_reason": "host unavailable", "host_os": "Darwin"}), encoding="utf-8")
    mismatched_host = root / "mismatched-host-matrix.json"
    result = subprocess.run([sys.executable, str(script), str(mismatched_host), "--summary", str(summary)], capture_output=True, text=True)
    if result.returncode == 0 or "does not match runner host" not in result.stderr + result.stdout:
        raise SystemExit("mismatched summary host was accepted")
    summary.write_text(json.dumps([{"qualified": False}]), encoding="utf-8")
    non_object = root / "non-object-matrix.json"
    result = subprocess.run([sys.executable, str(script), str(non_object), "--summary", str(summary)], capture_output=True, text=True)
    if result.returncode == 0 or "summary must be an object" not in result.stderr + result.stdout:
        raise SystemExit("non-object summary was accepted")
    summary.write_text("{malformed", encoding="utf-8")
    malformed_summary = root / "malformed-summary-matrix.json"
    result = subprocess.run([sys.executable, str(script), str(malformed_summary), "--summary", str(summary)], capture_output=True, text=True)
    if result.returncode == 0 or "malformed native qualification summary" not in result.stderr + result.stdout:
        raise SystemExit("malformed summary JSON was not reported cleanly")
    outside = root / "outside-matrix.json"
    outside.write_text("existing", encoding="utf-8")
    linked_output = root / "linked-matrix.json"
    try:
        linked_output.symlink_to(outside)
    except OSError:
        pass
    else:
        result = subprocess.run([sys.executable, str(script), str(linked_output)], capture_output=True, text=True)
        if result.returncode == 0 or "symbolic link" not in result.stderr + result.stdout:
            raise SystemExit("symbolic-link matrix output was accepted")
print("PipeWire qualification matrix behavior passed (2 scenarios)")
