"""Validate a host provenance object against the qualification contract."""
import json
import sys
from pathlib import Path

if len(sys.argv) not in (2, 3):
    raise SystemExit("usage: validate_native_host_provenance.py HOST.json [REPORT.json]")
host = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
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
    Path(sys.argv[2]).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
print("native host provenance verified")
