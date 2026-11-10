"""Validate a host provenance object against the qualification contract."""
import json
import os
import sys
import tempfile
from pathlib import Path

if len(sys.argv) not in (2, 3):
    raise SystemExit("usage: validate_native_host_provenance.py HOST.json [REPORT.json]")
try:
    def reject_duplicate_keys(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"duplicate key: {key}")
            result[key] = value
        return result
    host = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"), object_pairs_hook=reject_duplicate_keys)
except (OSError, UnicodeError, json.JSONDecodeError) as error:
    raise SystemExit(f"host provenance is unreadable: {error}")
except ValueError as error:
    raise SystemExit(f"host provenance is ambiguous: {error}")
if not isinstance(host, dict):
    raise SystemExit("host provenance must be an object")
if host.get("platform") not in ("linux", "windows", "macos"):
    raise SystemExit("host provenance has an invalid platform")
if not isinstance(host.get("name"), str) or not host["name"].strip():
    raise SystemExit("host provenance has no name")
if host.get("node_id") is not None and (not isinstance(host["node_id"], int) or isinstance(host["node_id"], bool) or host["node_id"] < 1):
    raise SystemExit("host provenance has an invalid node id")
if host.get("secure_boot") is not None and not isinstance(host["secure_boot"], bool):
    raise SystemExit("host provenance has an invalid secure boot flag")
if host.get("tpm_present") is not None and not isinstance(host["tpm_present"], bool):
    raise SystemExit("host provenance has an invalid TPM flag")
if len(sys.argv) == 3:
    report = {"schema": "beatglass-native-host-provenance-v1", "valid": True, "platform": host["platform"], "name": host["name"], "checked_fields": sorted(host)}
    try:
        destination = Path(sys.argv[2])
        destination.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", dir=destination.parent, delete=False) as temporary:
            temporary.write(json.dumps(report, indent=2) + "\n")
            temporary.flush()
            os.fsync(temporary.fileno())
            temporary_path = Path(temporary.name)
        os.replace(temporary_path, destination)
    except OSError as error:
        if "temporary_path" in locals():
            temporary_path.unlink(missing_ok=True)
        raise SystemExit(f"host provenance report is unwritable: {error}")
print("native host provenance verified")
