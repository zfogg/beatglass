"""Exercise optional post-discontinuity reacquisition evidence validation."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).parents[2]
VALIDATOR = ROOT / "tests" / "pipewire" / "validate_pipewire_transition_evidence.py"
events = [
    {"event": "start", "epoch": 1, "serial": "sink-a"},
    {"event": "process", "epoch": 1, "sample_count": 256},
    {"event": "error", "epoch": 1, "discontinuity": True, "error_code": -32},
    {"event": "restart", "epoch": 2, "serial": "sink-b", "format": {"sample_rate": 48000, "channels": 2}},
]


def run(reacquisition):
    payload = {"schema": "beatglass-pipewire-transitions-v1", "commit": "test", "events": json.loads(json.dumps(events))}
    payload["events"][-1]["reacquisition"] = reacquisition
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "transitions.json"
        path.write_text(json.dumps(payload), encoding="utf-8")
        return subprocess.run([sys.executable, str(VALIDATOR), str(path)], capture_output=True, text=True)


valid = run({"epoch": 2, "serial": "sink-b", "sample_count": 128})
assert valid.returncode == 0, valid.stderr
wrong_epoch = run({"epoch": 1, "serial": "sink-b", "sample_count": 128})
assert wrong_epoch.returncode != 0
assert "wrong epoch" in wrong_epoch.stderr
empty = run({"epoch": 2, "serial": "sink-b", "sample_count": 0})
assert empty.returncode != 0
assert "captured samples" in empty.stderr
print("PipeWire reacquisition behavior verified (3 scenarios)")
