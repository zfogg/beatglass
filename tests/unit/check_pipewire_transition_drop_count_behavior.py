"""Exercise optional process drop-count validation."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).parents[2]
VALIDATOR = ROOT / "tests" / "pipewire" / "validate_pipewire_transition_evidence.py"
base = [
    {"event": "start", "epoch": 1, "serial": "sink-a"},
    {"event": "process", "epoch": 1, "sample_count": 256, "drop_count": 0},
    {"event": "error", "epoch": 1, "discontinuity": True, "error_code": -32},
    {"event": "restart", "epoch": 2, "serial": "sink-a", "format": {"sample_rate": 48000, "channels": 2}},
]


def run(events):
    payload = {"schema": "beatglass-pipewire-transitions-v1", "commit": "test", "events": events}
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "transitions.json"
        path.write_text(json.dumps(payload), encoding="utf-8")
        return subprocess.run([sys.executable, str(VALIDATOR), str(path)], capture_output=True, text=True)


assert run(base).returncode == 0
missing = json.loads(json.dumps(base))
missing.append({"event": "process", "epoch": 1, "sample_count": 128})
result = run(missing)
assert result.returncode != 0
assert result.stderr
negative = json.loads(json.dumps(base))
negative[1]["drop_count"] = -1
result = run(negative)
assert result.returncode != 0
assert result.stderr
print("PipeWire drop-count behavior verified (3 scenarios)")
