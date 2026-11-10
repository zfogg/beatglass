"""Exercise sample-position/sample-count coupling validation."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).parents[2]
VALIDATOR = ROOT / "tests" / "pipewire" / "validate_pipewire_transition_evidence.py"
events = [
    {"event": "start", "epoch": 1, "serial": "sink-a"},
    {"event": "process", "epoch": 1, "sample_position": 0, "sample_count": 256},
    {"event": "error", "epoch": 1, "discontinuity": True, "error_code": -32},
    {"event": "restart", "epoch": 2, "serial": "sink-a", "format": {"sample_rate": 48000, "channels": 2}},
]


def run(items):
    payload = {"schema": "beatglass-pipewire-transitions-v1", "commit": "test", "events": items}
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "transitions.json"
        path.write_text(json.dumps(payload), encoding="utf-8")
        return subprocess.run([sys.executable, str(VALIDATOR), str(path)], capture_output=True, text=True)


assert run(events).returncode == 0
missing = json.loads(json.dumps(events))
del missing[1]["sample_count"]
result = run(missing)
assert result.returncode != 0
assert "captured samples" in result.stderr
zero = json.loads(json.dumps(events))
zero[1]["sample_count"] = 0
result = run(zero)
assert result.returncode != 0
assert "captured samples" in result.stderr
print("PipeWire sample-position count behavior verified (3 scenarios)")
