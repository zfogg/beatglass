"""Validate the shape of a captured PipeWire qualification manifest."""
import json
import sys
from datetime import datetime
from pathlib import Path

if len(sys.argv) != 2:
    raise SystemExit("usage: validate_pipewire_manifest.py MANIFEST")
manifest_path = Path(sys.argv[1])
if manifest_path.is_symlink():
    raise SystemExit("refusing symlinked manifest")
try:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
except (OSError, json.JSONDecodeError) as exc:
    raise SystemExit(f"invalid manifest: {exc}")
required = ("schema_version", "captured_at_utc", "host_platform", "python_version", "pipewire_cli_version", "inventory")
if not isinstance(manifest, dict) or any(key not in manifest or manifest[key] is None for key in required):
    raise SystemExit("manifest missing required qualification identity or inventory")
if manifest["schema_version"] != 1:
    raise SystemExit("unsupported PipeWire qualification manifest schema")
if not str(manifest["host_platform"]).startswith("Linux"):
    raise SystemExit("manifest host_platform must identify Linux")
try:
    captured_at = datetime.fromisoformat(manifest["captured_at_utc"])
except ValueError:
    raise SystemExit("manifest captured_at_utc must be ISO-8601")
if captured_at.utcoffset() is None:
    raise SystemExit("manifest captured_at_utc must include a timezone")
if captured_at.utcoffset().total_seconds() != 0:
    raise SystemExit("manifest captured_at_utc must be UTC")
if not isinstance(manifest["inventory"], list):
    raise SystemExit("manifest inventory must be a list")
if any(not isinstance(item, dict) for item in manifest["inventory"]):
    raise SystemExit("manifest inventory entries must be objects")
if any(not isinstance(item.get("info"), dict) for item in manifest["inventory"]):
    raise SystemExit("manifest inventory entries must retain info objects")
if any(not isinstance(item.get("id"), int) or item["id"] < 0 for item in manifest["inventory"]):
    raise SystemExit("manifest inventory entries must have non-negative numeric ids")
ids = [item["id"] for item in manifest["inventory"]]
if len(ids) != len(set(ids)):
    raise SystemExit("manifest inventory object ids must be unique")
print("PipeWire qualification manifest is valid")
