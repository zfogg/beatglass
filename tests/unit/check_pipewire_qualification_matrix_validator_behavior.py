"""Exercise valid and invalid qualification matrix handoffs."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

validator = Path(__file__).parents[1] / "pipewire" / "validate_qualification_matrix.py"
valid = {"schema":"beatglass-native-qualification-matrix-v1", "host_os":"Windows", "commit":"native-commit", "platforms": {
    "Linux":{"status":"pending","evidence":[],"pending_reason":"host unavailable"},
    "Windows":{"status":"qualified","evidence":["windows.zip"],"native_execution_attempted":True},
    "Darwin":{"status":"pending","evidence":[],"pending_reason":"host unavailable"}}}
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / "matrix.json"
    path.write_text(json.dumps(valid), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    if result.returncode != 0:
        raise SystemExit(result.stderr or result.stdout)
    invalid = dict(valid, platforms={"Linux": valid["platforms"]["Linux"]})
    path.write_text(json.dumps(invalid), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    if result.returncode == 0 or "platform set is incomplete" not in result.stderr + result.stdout:
        raise SystemExit("incomplete platform matrix was accepted")
    invalid = json.loads(json.dumps(valid))
    invalid["platforms"]["Windows"]["evidence"] = []
    path.write_text(json.dumps(invalid), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    if result.returncode == 0 or "qualified platform has no evidence" not in result.stderr + result.stdout:
        raise SystemExit("qualified platform without evidence was accepted")
    invalid["platforms"]["Windows"]["evidence"] = [42]
    path.write_text(json.dumps(invalid), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    if result.returncode == 0 or "invalid evidence path" not in result.stderr + result.stdout:
        raise SystemExit("invalid evidence path was accepted")
    invalid["platforms"]["Windows"]["evidence"] = ["evidence\u0001.zip"]
    path.write_text(json.dumps(invalid), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    if result.returncode == 0 or "invalid evidence path" not in result.stderr + result.stdout:
        raise SystemExit("control-character evidence path was accepted")
    invalid["platforms"]["Windows"]["evidence"] = ["evidence.zip", "evidence.zip"]
    path.write_text(json.dumps(invalid), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    if result.returncode == 0 or "duplicate evidence path" not in result.stderr + result.stdout:
        raise SystemExit("duplicate evidence path was accepted")
    invalid["platforms"]["Windows"]["execution_blockers"] = ["desktop audio permission"]
    invalid["platforms"]["Windows"]["evidence"] = ["evidence.zip"]
    invalid["host_os"] = "Windows"
    path.write_text(json.dumps(invalid), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    if result.returncode == 0 or "qualified platform retains execution blockers" not in result.stderr + result.stdout:
        raise SystemExit("qualified platform blockers were accepted")
    invalid["platforms"]["Windows"]["execution_blockers"] = []
    invalid["platforms"]["Windows"]["evidence"] = ["../outside.zip"]
    path.write_text(json.dumps(invalid), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    if result.returncode == 0 or "invalid evidence path" not in result.stderr + result.stdout: raise SystemExit("traversal evidence path was accepted")
    invalid["platforms"]["Windows"]["evidence"] = ["evidence.zip"]
    invalid["host_os"] = 42
    path.write_text(json.dumps(invalid), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path)], capture_output=True, text=True)
    if result.returncode == 0 or "invalid qualification matrix host" not in result.stderr + result.stdout:
        raise SystemExit("invalid matrix host was accepted")
    evidence_root = Path(directory) / "evidence"
    evidence_root.mkdir()
    (evidence_root / "windows.zip").write_bytes(b"evidence")
    path.write_text(json.dumps(valid), encoding="utf-8")
    result = subprocess.run([sys.executable, str(validator), str(path), str(evidence_root)], capture_output=True, text=True)
    if result.returncode != 0:
        raise SystemExit(result.stderr or result.stdout)
    (evidence_root / "windows.zip").unlink()
    result = subprocess.run([sys.executable, str(validator), str(path), str(evidence_root)], capture_output=True, text=True)
    if result.returncode == 0 or "missing evidence artifact" not in result.stderr + result.stdout:
        raise SystemExit("missing evidence artifact was accepted")
    outside = Path(directory) / "outside.zip"
    outside.write_bytes(b"outside")
    link = evidence_root / "linked.zip"
    try:
        link.symlink_to(outside)
    except OSError:
        pass
    else:
        valid["platforms"]["Windows"]["evidence"] = ["linked.zip"]
        path.write_text(json.dumps(valid), encoding="utf-8")
        result = subprocess.run([sys.executable, str(validator), str(path), str(evidence_root)], capture_output=True, text=True)
        if result.returncode == 0 or "missing evidence artifact" not in result.stderr + result.stdout:
            raise SystemExit("outside-root symlink evidence was accepted")
print("PipeWire qualification matrix validator behavior passed (9 scenarios)")
