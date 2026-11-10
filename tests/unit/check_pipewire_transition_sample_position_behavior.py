"""Exercise optional sample-position continuity validation."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).parents[2]
VALIDATOR = ROOT / "tests" / "pipewire" / "validate_pipewire_transition_evidence.py"
base = [
    {"event": "start", "epoch": 1, "serial": "sink-a", "sample_position": 0},
    {"event": "process", "epoch": 1, "sample_position": 0, "sample_count": 256},
    {"event": "error", "epoch": 1, "discontinuity": True, "error_code": -32, "sample_position": 256},
    {"event": "process", "epoch": 2, "sample_position": 256, "sample_count": 128},
    {"event": "restart", "epoch": 2, "serial": "sink-a", "sample_position": 384, "format": {"sample_rate": 48000, "channels": 2}},
]


def run(events):
    payload = {"schema": "beatglass-pipewire-transitions-v1", "commit": "test", "events": events}
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "transitions.json"
        path.write_text(json.dumps(payload), encoding="utf-8")
        return subprocess.run([sys.executable, str(VALIDATOR), str(path)], capture_output=True, text=True)


assert run(base).returncode == 0
overlap = json.loads(json.dumps(base))
overlap[3]["sample_position"] = 128
result = run(overlap)
assert result.returncode != 0
assert "monotonic" in result.stderr or "overlap" in result.stderr
incomplete = json.loads(json.dumps(base))
del incomplete[2]["sample_position"]
result = run(incomplete)
assert result.returncode != 0
assert "complete" in result.stderr
print("PipeWire sample-position behavior verified (3 scenarios)")
