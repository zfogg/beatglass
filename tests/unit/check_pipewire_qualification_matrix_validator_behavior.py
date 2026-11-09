"""Exercise valid and invalid qualification matrix handoffs."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

validator = Path(__file__).parents[1] / "pipewire" / "validate_qualification_matrix.py"
valid = {"schema":"beatglass-native-qualification-matrix-v1", "platforms": {
    "Linux":{"status":"pending","evidence":[],"pending_reason":"host unavailable"},
    "Windows":{"status":"qualified","evidence":["windows.zip"]},
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
print("PipeWire qualification matrix validator behavior passed (3 scenarios)")
