"""Validate a non-fabricating native host capture plan."""
import json
import sys
from pathlib import Path

if len(sys.argv) != 2:
    raise SystemExit("usage: validate_native_host_capture_plan.py PLAN.json")
try:
    plan = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
except (OSError, json.JSONDecodeError, UnicodeDecodeError):
    raise SystemExit("malformed native host capture plan")
if plan.get("schema") != "beatglass-native-host-capture-plan-v1" or plan.get("fabricated") is not False:
    raise SystemExit("invalid or fabricated native host capture plan")
required = {"Linux", "Windows", "Darwin"}
if set(plan.get("platforms", {})) != required:
    raise SystemExit("native host capture plan is incomplete")
for name, entry in plan["platforms"].items():
    if entry.get("status") != "pending" or not entry.get("command") or not isinstance(entry.get("requires"), list) or not entry["requires"]:
        raise SystemExit(f"invalid native host capture entry: {name}")
print("Native host capture plan verified (3 platforms)")
