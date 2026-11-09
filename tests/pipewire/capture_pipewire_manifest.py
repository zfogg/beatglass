"""Capture immutable PipeWire inventory plus qualification tool identity."""
import json
import hashlib
import os
import platform
import subprocess
import sys
import tempfile
from datetime import datetime, timezone
from pathlib import Path

if platform.system() != "Linux":
    print("PipeWire manifest capture pending: run on Linux")
    raise SystemExit(0)
if len(sys.argv) != 2:
    raise SystemExit("usage: capture_pipewire_manifest.py OUTPUT")
output = Path(sys.argv[1])
if output.exists() or output.is_symlink():
    raise SystemExit("refusing to overwrite existing manifest")
dump = subprocess.run(["pw-dump"], check=False, capture_output=True, text=True)
if dump.returncode:
    raise SystemExit(f"pw-dump failed: {dump.stderr.strip()}")
try:
    inventory = json.loads(dump.stdout)
except json.JSONDecodeError as exc:
    raise SystemExit(f"pw-dump returned invalid JSON: {exc}")
if not isinstance(inventory, list):
    raise SystemExit("pw-dump returned a non-list inventory")
if not any(isinstance(item, dict) and item.get("info", {}).get("props", {}).get("media.class") == "Audio/Sink" for item in inventory):
    raise SystemExit("pw-dump returned no Audio/Sink")
version = subprocess.run(["pw-cli", "--version"], check=False,
                         capture_output=True, text=True).stdout.strip()
manifest = {"schema_version": 1,
            "captured_at_utc": datetime.now(timezone.utc).isoformat(),
            "host_platform": platform.platform(),
            "python_version": platform.python_version(),
            "pipewire_cli_version": version,
            "audio_sink_count": sum(1 for item in inventory if isinstance(item, dict) and item.get("info", {}).get("props", {}).get("media.class") == "Audio/Sink"),
            "inventory": inventory}
manifest["inventory_sha256"] = hashlib.sha256(json.dumps(inventory, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
output.parent.mkdir(parents=True, exist_ok=True)
with tempfile.NamedTemporaryFile("w", encoding="utf-8", dir=output.parent,
                                 prefix=f".{output.name}.", suffix=".tmp", delete=False) as temporary:
    temporary.write(json.dumps(manifest, indent=2) + "\n")
    temporary_name = temporary.name
try:
    os.link(temporary_name, output)
except FileExistsError:
    raise SystemExit("refusing to overwrite existing manifest")
finally:
    Path(temporary_name).unlink(missing_ok=True)
print(f"wrote PipeWire qualification manifest: {output}")
