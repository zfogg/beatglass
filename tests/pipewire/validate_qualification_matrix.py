"""Validate a cross-platform native qualification matrix handoff."""
import json
import sys
from pathlib import Path

if len(sys.argv) not in (2, 3):
    raise SystemExit("usage: validate_qualification_matrix.py MATRIX.json [EVIDENCE_ROOT]")
evidence_root = Path(sys.argv[2]).resolve() if len(sys.argv) == 3 else None
try:
    matrix = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
except (OSError, json.JSONDecodeError, UnicodeDecodeError):
    raise SystemExit("malformed qualification matrix")
if not isinstance(matrix, dict) or matrix.get("schema") != "beatglass-native-qualification-matrix-v1":
    raise SystemExit("invalid qualification matrix schema")
if "commit" in matrix and (not isinstance(matrix["commit"], str) or not matrix["commit"].strip()):
    raise SystemExit("invalid qualification matrix commit")
if not isinstance(matrix.get("host_os"), str) or matrix["host_os"] not in {"Linux", "Windows", "Darwin"}:
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
    if entry["status"] == "qualified" and not isinstance(matrix.get("commit"), str):
        raise SystemExit(f"qualified platform lacks evidence provenance: {name}")
    if any(not isinstance(path, str) or not path.strip() or path.startswith(("/", "\\")) or ".." in Path(path).parts or any(ord(char) < 32 for char in path) for path in entry["evidence"]):
        raise SystemExit(f"invalid evidence path: {name}")
    if evidence_root is not None:
        for path in entry["evidence"]:
            artifact = evidence_root / path
            try:
                contained = artifact.resolve().is_relative_to(evidence_root)
            except (OSError, ValueError):
                contained = False
            if not contained or not artifact.is_file():
                raise SystemExit(f"missing evidence artifact: {name}: {path}")
    if len(set(entry["evidence"])) != len(entry["evidence"]):
        raise SystemExit(f"duplicate evidence path: {name}")
    if entry["status"] == "pending" and not isinstance(entry.get("pending_reason"), str):
        raise SystemExit(f"pending qualification lacks reason: {name}")
    if "native_execution_attempted" in entry and not isinstance(entry["native_execution_attempted"], bool):
        raise SystemExit(f"invalid native execution attempt flag: {name}")
    if entry["status"] == "qualified" and entry.get("native_execution_attempted") is not True:
        raise SystemExit(f"qualified platform lacks native execution attempt: {name}")
    if entry["status"] == "qualified" and entry.get("execution_blockers"):
        raise SystemExit(f"qualified platform retains execution blockers: {name}")
    if "execution_blockers" in entry and (not isinstance(entry["execution_blockers"], list) or
            any(not isinstance(blocker, str) or not blocker.strip() for blocker in entry["execution_blockers"])):
        raise SystemExit(f"invalid native execution blockers: {name}")
print(f"Qualification matrix verified ({len(platforms)} platforms)")
