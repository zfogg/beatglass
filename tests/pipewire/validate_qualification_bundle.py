"""Validate a matrix plus rendered report as one portable handoff bundle."""
import json
import sys
from pathlib import Path

if len(sys.argv) != 3:
    raise SystemExit("usage: validate_qualification_bundle.py MATRIX.json REPORT.md")
try:
    matrix = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
    report = Path(sys.argv[2]).read_text(encoding="utf-8")
except (OSError, json.JSONDecodeError, UnicodeDecodeError):
    raise SystemExit("malformed qualification bundle")
if matrix.get("schema") != "beatglass-native-qualification-matrix-v1" or "# Native qualification matrix" not in report:
    raise SystemExit("invalid qualification bundle schema")
for name, entry in matrix.get("platforms", {}).items():
    if f"| {name} | **{entry.get('status')}** |" not in report:
        raise SystemExit(f"qualification report is missing platform: {name}")
print("Qualification bundle verified")
