"""Create an auditable cross-platform native qualification status matrix."""
import json
import platform
import sys
from pathlib import Path

if len(sys.argv) not in (2, 4) or (len(sys.argv) == 4 and sys.argv[2] != "--summary"):
    raise SystemExit("usage: qualification_matrix.py OUTPUT.json [--summary SUMMARY.json]")
output = Path(sys.argv[1])
summary = json.loads(Path(sys.argv[3]).read_text(encoding="utf-8")) if len(sys.argv) == 4 else None
host = platform.system()
platforms = {
    "Linux": {"target": "Linux/PipeWire", "status": "pending", "evidence": []},
    "Windows": {"target": "Windows/native audio/overlay", "status": "pending", "evidence": []},
    "Darwin": {"target": "macOS/native audio/overlay", "status": "pending", "evidence": []},
}
for item in platforms.values():
    if item["status"] == "pending":
        item["pending_reason"] = "native host execution pending"
if summary is not None:
    current = platforms.get(host)
    if current is not None:
        current["status"] = "qualified" if summary.get("qualified") is True else "pending"
        current["evidence"] = summary.get("native_artifacts", [])
        if current["status"] == "pending":
            current["pending_reason"] = summary.get("pending_reason", "native qualification incomplete")
matrix = {"schema": "beatglass-native-qualification-matrix-v1", "host_os": host, "python": sys.version.split()[0], "platforms": platforms}
if summary is not None and isinstance(summary.get("commit"), str) and summary["commit"].strip():
    matrix["commit"] = summary["commit"]
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(matrix, indent=2) + "\n", encoding="utf-8")
print(f"Wrote qualification matrix for {len(platforms)} platforms to {output}")
