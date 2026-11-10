"""Exercise active sink identity validation for error events."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).parents[2]
VALIDATOR = ROOT / "tests" / "pipewire" / "validate_pipewire_transition_evidence.py"
base = [
    {"event": "start", "epoch": 1, "serial": "sink-a"},
    {"event": "process", "epoch": 1, "sample_count": 256},
    {"event": "error", "epoch": 1, "serial": "sink-a", "discontinuity": True, "error_code": -32},
    {"event": "restart", "epoch": 2, "serial": "sink-b", "format": {"sample_rate": 48000, "channels": 2}},
]


def run(events):
    payload = {"schema": "beatglass-pipewire-transitions-v1", "commit": "test", "events": events}
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "transitions.json"
        path.write_text(json.dumps(payload), encoding="utf-8")
        return subprocess.run([sys.executable, str(VALIDATOR), str(path)], capture_output=True, text=True)


assert run(base).returncode == 0
wrong = json.loads(json.dumps(base))
wrong[2]["serial"] = "sink-old"
result = run(wrong)
assert result.returncode != 0
assert "active sink" in result.stderr
print("PipeWire error serial behavior verified (2 scenarios)")
