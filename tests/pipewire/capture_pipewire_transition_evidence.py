"""Attach host/checkout provenance to raw native transition events."""
import argparse
import json
import platform
import sys
import subprocess
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("input", type=Path, help="raw JSON event list captured from the daemon")
parser.add_argument("output", type=Path)
args = parser.parse_args()
events = json.loads(args.input.read_text(encoding="utf-8"))
if not isinstance(events, list) or not events:
    raise SystemExit("raw transition capture must be a non-empty event list")
commit = subprocess.run(["git", "rev-parse", "HEAD"], capture_output=True, text=True, check=False).stdout.strip()
if not commit:
    raise SystemExit("could not determine checkout commit")
payload = {"schema": "beatglass-pipewire-transitions-v1", "platform": platform.platform(),
           "commit": commit, "events": events}
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
validator = Path(__file__).with_name("validate_pipewire_transition_evidence.py")
result = subprocess.run([sys.executable, str(validator), str(args.output)], capture_output=True, text=True, check=False)
if result.returncode:
    args.output.unlink(missing_ok=True)
    raise SystemExit(result.stderr or result.stdout or "captured transition evidence failed validation")
print(f"Captured {len(events)} PipeWire transition events with commit {commit}")
