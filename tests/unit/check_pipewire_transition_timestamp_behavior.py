"""Exercise timestamp validation for daemon-captured PipeWire transitions."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).parents[2]
VALIDATOR = ROOT / "tests" / "pipewire" / "validate_pipewire_transition_evidence.py"


def payload(timestamps):
    events = [
        {"event": "start", "epoch": 1, "serial": "sink-a"},
        {"event": "process", "epoch": 1, "sample_count": 256},
        {"event": "error", "epoch": 1, "discontinuity": True, "error_code": -32},
        {"event": "restart", "epoch": 2, "serial": "sink-a", "format": {"sample_rate": 48000, "channels": 2}},
    ]
    for event, timestamp in zip(events, timestamps):
        event["timestamp_ns"] = timestamp
    return {"schema": "beatglass-pipewire-transitions-v1", "commit": "test", "events": events}


def run(data):
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "transitions.json"
        path.write_text(json.dumps(data), encoding="utf-8")
        return subprocess.run([sys.executable, str(VALIDATOR), str(path)], capture_output=True, text=True)


valid = run(payload([10, 20, 30, 40]))
assert valid.returncode == 0, valid.stderr

non_monotonic = run(payload([10, 30, 20, 40]))
assert non_monotonic.returncode != 0
assert "monotonic" in non_monotonic.stderr

incomplete = payload([10, 20, 30, 40])
del incomplete["events"][-1]["timestamp_ns"]
missing = run(incomplete)
assert missing.returncode != 0
assert "complete" in missing.stderr

print("PipeWire transition timestamp behavior verified (3 scenarios)")
