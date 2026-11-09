"""Capture a timestamped pw-dump inventory for native qualification evidence."""
import argparse
import platform
import subprocess
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
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(result.stdout, encoding="utf-8")
print(f"wrote PipeWire inventory: {args.output}")
