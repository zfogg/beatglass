"""Exercise optional transition artifact identity validation."""
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
    {"event": "restart", "epoch": 2, "serial": "sink-a", "format": {"sample_rate": 48000, "channels": 2}},
]


def run(artifact_id):
    payload = {"schema": "beatglass-pipewire-transitions-v1", "commit": "test", "artifact_id": artifact_id, "events": events}
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "transitions.json"
        path.write_text(json.dumps(payload), encoding="utf-8")
        return subprocess.run([sys.executable, str(VALIDATOR), str(path)], capture_output=True, text=True)


assert run("capture-2026-11-09T23:00:00Z").returncode == 0
result = run("")
assert result.returncode != 0
assert "artifact identity" in result.stderr
result = run(42)
assert result.returncode != 0
assert "artifact identity" in result.stderr
print("PipeWire artifact identity behavior verified (3 scenarios)")
