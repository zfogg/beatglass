"""Validate a generated native host provenance report."""
import json
import sys
from pathlib import Path

if len(sys.argv) != 2:
    raise SystemExit("usage: validate_native_host_provenance_report.py REPORT.json")
try:
    def reject_duplicate_keys(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"duplicate key: {key}")
            result[key] = value
        return result

    report = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"), object_pairs_hook=reject_duplicate_keys)
except (OSError, UnicodeError, json.JSONDecodeError) as error:
    raise SystemExit(f"host provenance report is unreadable: {error}")
except ValueError as error:
    raise SystemExit(f"host provenance report is ambiguous: {error}")
if not isinstance(report, dict) or report.get("schema") != "beatglass-native-host-provenance-v1":
    raise SystemExit("host provenance report has an unsupported schema")
if set(report) != {"schema", "valid", "platform", "name", "checked_fields"}:
    raise SystemExit("host provenance report has unsupported fields")
if report.get("valid") is not True:
    raise SystemExit("host provenance report is not valid")
if report.get("platform") not in ("linux", "windows", "macos"):
    raise SystemExit("host provenance report has an invalid platform")
if not isinstance(report.get("name"), str) or not report["name"].strip():
    raise SystemExit("host provenance report has no name")
fields = report.get("checked_fields")
if not isinstance(fields, list) or any(not isinstance(value, str) or not value for value in fields):
    raise SystemExit("host provenance report has invalid checked fields")
if fields != sorted(fields) or len(fields) != len(set(fields)):
    raise SystemExit("host provenance report has invalid checked fields")
if "platform" not in fields or "name" not in fields:
    raise SystemExit("host provenance report omits required provenance fields")
print("native host provenance report verified")
