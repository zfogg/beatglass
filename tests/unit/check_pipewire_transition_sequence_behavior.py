"""Exercise optional contiguous event sequence validation."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).parents[2]
VALIDATOR = ROOT / "tests" / "pipewire" / "validate_pipewire_transition_evidence.py"
base = [
    {"event": "start", "epoch": 1, "serial": "sink-a", "sequence": 0},
    {"event": "process", "epoch": 1, "sample_count": 256, "sequence": 1},
    {"event": "error", "epoch": 1, "discontinuity": True, "error_code": -32, "sequence": 2},
    {"event": "restart", "epoch": 2, "serial": "sink-a", "sequence": 3, "format": {"sample_rate": 48000, "channels": 2}},
]


def run(events):
    payload = {"schema": "beatglass-pipewire-transitions-v1", "commit": "test", "events": events}
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "transitions.json"
        path.write_text(json.dumps(payload), encoding="utf-8")
        return subprocess.run([sys.executable, str(VALIDATOR), str(path)], capture_output=True, text=True)


assert run(base).returncode == 0
gap = json.loads(json.dumps(base))
gap[2]["sequence"] = 4
result = run(gap)
assert result.returncode != 0
assert "contiguous" in result.stderr
missing = json.loads(json.dumps(base))
del missing[1]["sequence"]
result = run(missing)
assert result.returncode != 0
assert "complete" in result.stderr
print("PipeWire event sequence behavior verified (3 scenarios)")
