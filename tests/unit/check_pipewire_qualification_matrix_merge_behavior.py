"""Exercise merging independent host qualification matrices."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

script = Path(__file__).parents[1] / "pipewire" / "merge_qualification_matrices.py"
base = {"schema":"beatglass-native-qualification-matrix-v1", "platforms": {
    "Linux":{"status":"qualified","evidence":["linux.zip"]},
    "Windows":{"status":"pending","evidence":[],"pending_reason":"host required"}}}
mac = {"schema":"beatglass-native-qualification-matrix-v1", "platforms": {
    "Darwin":{"status":"qualified","evidence":["mac.zip"]}}}
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    left, right, output = root / "left.json", root / "right.json", root / "merged.json"
    left.write_text(json.dumps(base), encoding="utf-8"); right.write_text(json.dumps(mac), encoding="utf-8")
    result = subprocess.run([sys.executable, str(script), str(output), str(left), str(right)], capture_output=True, text=True)
    if result.returncode != 0: raise SystemExit(result.stderr or result.stdout)
    if set(json.loads(output.read_text(encoding="utf-8"))["platforms"]) != {"Linux", "Windows", "Darwin"}:
        raise SystemExit("merged matrix omitted a platform")
print("PipeWire qualification matrix merge behavior passed (1 scenario)")
