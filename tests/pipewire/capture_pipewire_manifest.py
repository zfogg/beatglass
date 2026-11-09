"""Capture immutable PipeWire inventory plus qualification tool identity."""
import json
import platform
import subprocess
import sys
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
version = subprocess.run(["pw-cli", "--version"], check=False,
                         capture_output=True, text=True).stdout.strip()
manifest = {"captured_at_utc": datetime.now(timezone.utc).isoformat(),
            "pipewire_cli_version": version, "inventory": inventory}
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
print(f"wrote PipeWire qualification manifest: {output}")
