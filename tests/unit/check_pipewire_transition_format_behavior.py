"""Exercise optional per-event negotiated format validation."""
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
    {"event": "error", "epoch": 1, "discontinuity": True, "error_code": -32},
    {"event": "restart", "epoch": 2, "serial": "sink-a", "format": {"sample_rate": 48000, "channels": 2}},
]


def run(events):
    payload = {"schema": "beatglass-pipewire-transitions-v1", "commit": "test", "events": events}
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "transitions.json"
        path.write_text(json.dumps(payload), encoding="utf-8")
        return subprocess.run([sys.executable, str(VALIDATOR), str(path)], capture_output=True, text=True)


matching = json.loads(json.dumps(base))
matching[1]["format"] = {"sample_rate": 48000, "channels": 2}
assert run(matching).returncode == 0
mismatch = json.loads(json.dumps(matching))
mismatch[1]["format"] = {"sample_rate": 44100, "channels": 2}
result = run(mismatch)
assert result.returncode != 0
assert "disagrees" in result.stderr
invalid = json.loads(json.dumps(matching))
invalid[1]["format"] = {"sample_rate": 0, "channels": 2}
result = run(invalid)
assert result.returncode != 0
assert "invalid" in result.stderr
print("PipeWire transition format behavior verified (3 scenarios)")
