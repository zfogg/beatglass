"""Capture a timestamped pw-dump inventory for native qualification evidence."""
import argparse
import json
import os
import platform
import subprocess
import tempfile
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("output", type=Path)
args = parser.parse_args()
if platform.system() != "Linux":
    print("PipeWire inventory capture pending: run on Linux")
    raise SystemExit(0)
result = subprocess.run(["pw-dump"], check=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
if result.returncode:
    raise SystemExit(f"pw-dump failed: {result.stderr.strip()}")
try:
    inventory = json.loads(result.stdout)
except json.JSONDecodeError as exc:
    raise SystemExit(f"pw-dump returned invalid JSON: {exc}")
if not isinstance(inventory, list):
    raise SystemExit("pw-dump returned a non-list inventory")
if args.output.is_symlink():
    raise SystemExit("refusing symlinked evidence output file")
if args.output.exists():
    raise SystemExit("refusing to overwrite existing evidence output")
if args.output.parent.exists() and args.output.parent.is_symlink():
    raise SystemExit("refusing symlinked evidence output directory")
args.output.parent.mkdir(parents=True, exist_ok=True)
with tempfile.NamedTemporaryFile(
    "w", encoding="utf-8", dir=args.output.parent,
    prefix=f".{args.output.name}.", suffix=".tmp", delete=False,
) as temporary:
    temporary.write(result.stdout)
    temporary_name = temporary.name
try:
    # A hard link creates the final evidence path without replacing a file
    # created by another qualification process between our preflight checks.
    os.link(temporary_name, args.output)
except FileExistsError:
    raise SystemExit("refusing to overwrite existing evidence output")
finally:
    Path(temporary_name).unlink(missing_ok=True)
print(f"wrote PipeWire inventory: {args.output}")
