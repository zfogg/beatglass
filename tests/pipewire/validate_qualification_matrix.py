"""Validate a cross-platform native qualification matrix handoff."""
import json
import sys
from pathlib import Path

if len(sys.argv) != 2:
    raise SystemExit("usage: validate_qualification_matrix.py MATRIX.json")
try:
    matrix = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
except (OSError, json.JSONDecodeError, UnicodeDecodeError):
    raise SystemExit("malformed qualification matrix")
if not isinstance(matrix, dict) or matrix.get("schema") != "beatglass-native-qualification-matrix-v1":
    raise SystemExit("invalid qualification matrix schema")
if "commit" in matrix and (not isinstance(matrix["commit"], str) or not matrix["commit"].strip()):
    raise SystemExit("invalid qualification matrix commit")
if not isinstance(matrix.get("host_os"), str) or not matrix["host_os"].strip():
    raise SystemExit("invalid qualification matrix host")
platforms = matrix.get("platforms")
required = {"Linux", "Windows", "Darwin"}
if not isinstance(platforms, dict) or set(platforms) != required:
    raise SystemExit("qualification matrix platform set is incomplete")
for name, entry in platforms.items():
    if not isinstance(entry, dict) or entry.get("status") not in {"pending", "qualified"} or not isinstance(entry.get("evidence"), list):
        raise SystemExit(f"invalid qualification matrix entry: {name}")
    if entry["status"] == "qualified" and not entry["evidence"]:
        raise SystemExit(f"qualified platform has no evidence: {name}")
    if any(not isinstance(path, str) or not path.strip() or any(ord(char) < 32 for char in path) for path in entry["evidence"]):
        raise SystemExit(f"invalid evidence path: {name}")
    if len(set(entry["evidence"])) != len(entry["evidence"]):
        raise SystemExit(f"duplicate evidence path: {name}")
    if entry["status"] == "pending" and not isinstance(entry.get("pending_reason"), str):
        raise SystemExit(f"pending qualification lacks reason: {name}")
print(f"Qualification matrix verified ({len(platforms)} platforms)")
