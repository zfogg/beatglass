"""Validate the shape of a captured PipeWire qualification manifest."""
import json
import sys
from pathlib import Path

if len(sys.argv) != 2:
    raise SystemExit("usage: validate_pipewire_manifest.py MANIFEST")
try:
    manifest = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
except (OSError, json.JSONDecodeError) as exc:
    raise SystemExit(f"invalid manifest: {exc}")
required = ("captured_at_utc", "host_platform", "python_version", "pipewire_cli_version", "inventory")
if not isinstance(manifest, dict) or any(not manifest.get(key) for key in required):
    raise SystemExit("manifest missing required qualification identity or inventory")
if not isinstance(manifest["inventory"], list):
    raise SystemExit("manifest inventory must be a list")
print("PipeWire qualification manifest is valid")
