"""Exercise optional host provenance validation for PipeWire transitions."""
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


def run(host):
    payload = {"schema": "beatglass-pipewire-transitions-v1", "commit": "test", "host": host, "events": events}
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "transitions.json"
        path.write_text(json.dumps(payload), encoding="utf-8")
        return subprocess.run([sys.executable, str(VALIDATOR), str(path)], capture_output=True, text=True)


valid = {"platform": "linux", "daemon_version": "1.2.3", "capture_command": "pw-cat --record"}
assert run(valid).returncode == 0
invalid_platform = dict(valid, platform="android")
result = run(invalid_platform)
assert result.returncode != 0
assert "invalid platform" in result.stderr
missing_command = dict(valid, capture_command="")
result = run(missing_command)
assert result.returncode != 0
assert "capture command" in result.stderr
print("PipeWire host provenance behavior verified (3 scenarios)")
